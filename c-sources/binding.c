#include <Python.h>
#include "quickjs.h"

typedef struct {
    PyObject_HEAD
    JSRuntime *rt;
    JSContext *ctx;
} Context;

static JSClassID js_py_class_id = 0;

static void js_py_finalizer(JSRuntime *rt, JSValue val) {
    PyObject *obj = (PyObject *)JS_GetOpaque(val, js_py_class_id);
    Py_XDECREF(obj);
}

static JSClassDef js_py_class = {
    "PythonObject",
    .finalizer = js_py_finalizer,
};

// Forward declarations
static JSValue py_to_js(JSContext *ctx, PyObject *obj);
static PyObject *js_to_py(JSContext *ctx, JSValueConst val);

// Proxy function for calling Python from JS
static JSValue python_callback_proxy(JSContext *ctx, JSValueConst this_val,
                                     int argc, JSValueConst *argv,
                                     int magic, JSValue *func_data) {
    // func_data[0] should be the wrapped Python callable
    PyObject *func = (PyObject *)JS_GetOpaque(func_data[0], js_py_class_id);
    if (!func) {
        return JS_ThrowInternalError(ctx, "Failed to get Python function");
    }

    PyObject *args = PyTuple_New(argc);
    for (int i = 0; i < argc; i++) {
        PyObject *arg = js_to_py(ctx, argv[i]);
        if (!arg) {
            Py_DECREF(args);
            return JS_ThrowInternalError(ctx, "Failed to convert argument %d", i);
        }
        PyTuple_SetItem(args, i, arg); // Steals reference
    }

    PyObject *result = PyObject_CallObject(func, args);
    Py_DECREF(args);

    if (!result) {
        // Python exception occurred
        PyObject *ptype, *pvalue, *ptraceback;
        PyErr_Fetch(&ptype, &pvalue, &ptraceback);
        PyObject *str_exc = PyObject_Str(pvalue);
        const char *msg = PyUnicode_AsUTF8(str_exc);
        JSValue err = JS_ThrowInternalError(ctx, "Python Exception: %s", msg);
        Py_XDECREF(str_exc);
        Py_XDECREF(ptype);
        Py_XDECREF(pvalue);
        Py_XDECREF(ptraceback);
        return err;
    }

    JSValue js_result = py_to_js(ctx, result);
    Py_DECREF(result);
    return js_result;
}

static JSValue py_to_js(JSContext *ctx, PyObject *obj) {
    if (obj == Py_None) {
        return JS_NULL;
    } else if (PyBool_Check(obj)) {
        return JS_NewBool(ctx, obj == Py_True);
    } else if (PyLong_Check(obj)) {
        return JS_NewInt64(ctx, PyLong_AsLongLong(obj));
    } else if (PyFloat_Check(obj)) {
        return JS_NewFloat64(ctx, PyFloat_AsDouble(obj));
    } else if (PyUnicode_Check(obj)) {
        const char *str = PyUnicode_AsUTF8(obj);
        return JS_NewString(ctx, str);
    } else if (PyCallable_Check(obj)) {
        // Wrap callable
        JSValue wrapper = JS_NewObjectClass(ctx, js_py_class_id);
        Py_INCREF(obj);
        JS_SetOpaque(wrapper, obj);
        
        JSValue func = JS_NewCFunctionData(ctx, python_callback_proxy, 0, 0, 1, &wrapper);
        JS_FreeValue(ctx, wrapper); // func has dup'd it (or rather, copied the value which is refcounted)
        return func;
    }
    // Fallback: string representation
    PyObject *str = PyObject_Str(obj);
    const char *c_str = PyUnicode_AsUTF8(str);
    JSValue val = JS_NewString(ctx, c_str);
    Py_DECREF(str);
    return val;
}

static PyObject *js_to_py(JSContext *ctx, JSValueConst val) {
    int tag = JS_VALUE_GET_TAG(val);
    if (JS_IsNumber(val)) {
        if (tag == JS_TAG_INT) {
             int64_t v;
             JS_ToInt64(ctx, &v, val);
             return PyLong_FromLongLong(v);
        } else {
             double v;
             JS_ToFloat64(ctx, &v, val);
             return PyFloat_FromDouble(v);
        }
    } else if (JS_IsString(val)) {
        const char *str = JS_ToCString(ctx, val);
        PyObject *res = PyUnicode_FromString(str);
        JS_FreeCString(ctx, str);
        return res;
    } else if (JS_IsBool(val)) {
        int v = JS_ToBool(ctx, val);
        return PyBool_FromLong(v);
    } else if (JS_IsNull(val) || JS_IsUndefined(val)) {
        Py_RETURN_NONE;
    } else if (JS_IsException(val)) {
        return NULL; // Caller handles
    }
    // Objects, Arrays, etc. -> Convert to string for now
    const char *str = JS_ToCString(ctx, val);
    PyObject *res = PyUnicode_FromString(str ? str : "[Object]");
    if (str) JS_FreeCString(ctx, str);
    return res;
}

static void
Context_dealloc(Context *self)
{
    if (self->ctx) JS_FreeContext(self->ctx);
    if (self->rt) JS_FreeRuntime(self->rt);
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *
Context_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Context *self;
    self = (Context *) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->rt = JS_NewRuntime();
        if (!self->rt) {
            Py_DECREF(self);
            return PyErr_NoMemory();
        }
        self->ctx = JS_NewContext(self->rt);
        if (!self->ctx) {
            Py_DECREF(self);
            return PyErr_NoMemory();
        }
        
        // Register the PythonObject class in this runtime
        if (!JS_IsRegisteredClass(self->rt, js_py_class_id)) {
             JS_NewClass(self->rt, js_py_class_id, &js_py_class);
        }
    }
    return (PyObject *) self;
}

static PyObject *
Context_set(Context *self, PyObject *args)
{
    const char *name;
    PyObject *value;
    if (!PyArg_ParseTuple(args, "sO", &name, &value))
        return NULL;

    JSValue js_val = py_to_js(self->ctx, value);
    JSValue global = JS_GetGlobalObject(self->ctx);
    JS_SetPropertyStr(self->ctx, global, name, js_val);
    JS_FreeValue(self->ctx, global);
    
    Py_RETURN_NONE;
}

static PyObject *
Context_eval(Context *self, PyObject *args)
{
    const char *script;
    const char *filename = "<input>";
    if (!PyArg_ParseTuple(args, "s|s", &script, &filename))
        return NULL;

    JSValue val = JS_Eval(self->ctx, script, strlen(script), filename, JS_EVAL_TYPE_GLOBAL);

    if (JS_IsException(val)) {
        JSValue exception_val = JS_GetException(self->ctx);
        const char *exception_str = JS_ToCString(self->ctx, exception_val);
        PyErr_SetString(PyExc_RuntimeError, exception_str ? exception_str : "Unknown QuickJS Exception");
        if (exception_str) JS_FreeCString(self->ctx, exception_str);
        JS_FreeValue(self->ctx, exception_val);
        JS_FreeValue(self->ctx, val);
        return NULL;
    }

    PyObject *res = js_to_py(self->ctx, val);
    JS_FreeValue(self->ctx, val);
    
    return res;
}

static PyMethodDef Context_methods[] = {
    {"eval", (PyCFunction) Context_eval, METH_VARARGS, "Evaluate JS code"},
    {"set", (PyCFunction) Context_set, METH_VARARGS, "Set a global variable"},
    {NULL}  /* Sentinel */
};

static PyTypeObject ContextType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_quickjs.Context",
    .tp_doc = "QuickJS Context",
    .tp_basicsize = sizeof(Context),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = Context_new,
    .tp_dealloc = (destructor) Context_dealloc,
    .tp_methods = Context_methods,
};

static struct PyModuleDef quickjsmodule = {
    PyModuleDef_HEAD_INIT,
    "_quickjs",
    "QuickJS bindings",
    -1,
    NULL
};

PyMODINIT_FUNC
PyInit__quickjs(void)
{
    PyObject *m;
    if (PyType_Ready(&ContextType) < 0)
        return NULL;

    m = PyModule_Create(&quickjsmodule);
    if (m == NULL)
        return NULL;

    Py_INCREF(&ContextType);
    if (PyModule_AddObject(m, "Context", (PyObject *) &ContextType) < 0) {
        Py_DECREF(&ContextType);
        Py_DECREF(m);
        return NULL;
    }
    
    // Initialize class ID
    JS_NewClassID(&js_py_class_id);

    return m;
}
