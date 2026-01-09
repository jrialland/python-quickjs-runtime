#include <Python.h>
#include "quickjs.h"

typedef struct
{
    PyObject_HEAD JSRuntime *rt;
} Runtime;
static PyTypeObject RuntimeType;

static JSClassID py_obj_class_id;
static void py_obj_finalizer(JSRuntime *rt, JSValue val)
{
    PyObject *obj = JS_GetOpaque(val, py_obj_class_id);
    if (obj)
    {
        PyGILState_STATE gstate = PyGILState_Ensure();
        Py_DECREF(obj);
        PyGILState_Release(gstate);
    }
}
static JSClassDef py_obj_class = {
    "PythonObject",
    .finalizer = py_obj_finalizer,
};

static PyObject *Runtime_SetRuntimeInfo(Runtime *self, PyObject *arg);
static PyObject *Runtime_SetMemoryLimit(Runtime *self, PyObject *arg);
static PyObject *Runtime_SetGCThreshold(Runtime *self, PyObject *arg);
static PyObject *Runtime_SetMaxStackSize(Runtime *self, PyObject *arg);
static PyObject *Runtime_UpdateStackTop(Runtime *self, PyObject *Py_UNUSED(ignored));
static PyObject *Runtime_RunGC(Runtime *self, PyObject *Py_UNUSED(ignored));
static PyObject *Runtime_NewContext(Runtime *self, PyObject *Py_UNUSED(ignored));
static PyObject *Runtime_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static void Runtime_dealloc(Runtime *self);


typedef struct
{
    PyObject_HEAD JSContext *ctx;
    Runtime *runtime;
} Context;
static PyTypeObject ContextType;
static PyObject *Context_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static void Context_dealloc(Context *self);
static PyObject *Context_Eval(Context *self, PyObject *args, PyObject *kwargs);
static PyObject *Context_EvalSync(Context *self, PyObject *args, PyObject *kwargs);
static PyObject *Context_Set(Context *self, PyObject *args);
static PyObject * Context_GetRuntime(Context *self, PyObject *Py_UNUSED(ignored));
static JSValue py_to_js_value(JSContext *ctx, PyObject *obj);
static JSValue py_callable_handler(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic, JSValue *func_data);

/*----------------------------------------------------------------------------*/
/* Runtime object representing a QuickJS runtime */

static PyObject *Runtime_SetRuntimeInfo(Runtime *self, PyObject *arg)
{
    const char *info = PyUnicode_AsUTF8(arg);
    if (!info)
    {
        return NULL;
    }
    JS_SetRuntimeInfo(self->rt, info);
    Py_RETURN_NONE;
}

static PyObject *Runtime_SetMemoryLimit(Runtime *self, PyObject *arg)
{
    size_t limit = PyLong_AsSize_t(arg);
    if (PyErr_Occurred())
    {
        return NULL;
    }
    JS_SetMemoryLimit(self->rt, limit);
    Py_RETURN_NONE;
}

static PyObject *Runtime_SetGCThreshold(Runtime *self, PyObject *arg)
{
    size_t gc_threshold = PyLong_AsSize_t(arg);
    if (PyErr_Occurred())
    {
        return NULL;
    }
    JS_SetGCThreshold(self->rt, gc_threshold);
    Py_RETURN_NONE;
}

static PyObject *Runtime_SetMaxStackSize(Runtime *self, PyObject *arg)
{
    size_t stack_size = PyLong_AsSize_t(arg);
    if (PyErr_Occurred())
    {
        return NULL;
    }
    JS_SetMaxStackSize(self->rt, stack_size);
    Py_RETURN_NONE;
}

static PyObject *Runtime_UpdateStackTop(Runtime *self, PyObject *Py_UNUSED(ignored))
{
    JS_UpdateStackTop(self->rt);
    Py_RETURN_NONE;
}

static PyObject *Runtime_RunGC(Runtime *self, PyObject *Py_UNUSED(ignored))
{
    JS_RunGC(self->rt);
    Py_RETURN_NONE;
}

static PyObject *Runtime_NewContext(Runtime *self, PyObject *Py_UNUSED(ignored))
{
    JSContext *ctx = JS_NewContext(self->rt);
    if (!ctx)
    {
        return PyErr_NoMemory();
    }
    Context *py_ctx = PyObject_New(Context, &ContextType);
    if (!py_ctx)
    {
        JS_FreeContext(ctx);
        return PyErr_NoMemory();
    }
    py_ctx->ctx = ctx;
    Py_INCREF(self);
    py_ctx->runtime = self;
    return (PyObject *)py_ctx;
}

static PyObject *Runtime_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Runtime *self;
    self = (Runtime *)type->tp_alloc(type, 0);
    if (self != NULL)
    {
        self->rt = JS_NewRuntime();
        if (!self->rt)
        {
            Py_DECREF(self);
            return PyErr_NoMemory();
        }
        JS_NewClass(self->rt, py_obj_class_id, &py_obj_class);
    }
    return (PyObject *)self;
}

static void Runtime_dealloc(Runtime *self)
{
    if (self->rt)
        JS_FreeRuntime(self->rt);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyMethodDef Runtime_methods[] = {
    {"set_runtime_info", (PyCFunction)Runtime_SetRuntimeInfo, METH_O, "Set runtime info"},
    {"set_memory_limit", (PyCFunction)Runtime_SetMemoryLimit, METH_O, "Set memory limit"},
    {"set_gc_threshold", (PyCFunction)Runtime_SetGCThreshold, METH_O, "Set GC threshold"},
    {"set_max_stack_size", (PyCFunction)Runtime_SetMaxStackSize, METH_O, "Set max stack size"},
    {"update_stack_top", (PyCFunction)Runtime_UpdateStackTop, METH_NOARGS, "Update stack top"},
    {"run_gc", (PyCFunction)Runtime_RunGC, METH_NOARGS, "Run garbage collector"},
    {"new_context", (PyCFunction)Runtime_NewContext, METH_NOARGS, "Create a new QuickJS Context"},
    {NULL} /* Sentinel */
};

static PyTypeObject RuntimeType = {
    PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "_quickjs.Runtime",
    .tp_doc = "QuickJS Runtime",
    .tp_basicsize = sizeof(Runtime),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = Runtime_new,
    .tp_dealloc = (destructor)Runtime_dealloc,
    .tp_methods = Runtime_methods,
};

/*----------------------------------------------------------------------------*/
/* Context object representing a QuickJS context */

static PyObject *js_value_to_py(JSContext *ctx, JSValue val)
{
    int tag = JS_VALUE_GET_NORM_TAG(val);
    switch (tag)
    {
    case JS_TAG_INT:
        return PyLong_FromLong(JS_VALUE_GET_INT(val));
    case JS_TAG_BOOL:
        return PyBool_FromLong(JS_VALUE_GET_BOOL(val));
    case JS_TAG_FLOAT64:
        return PyFloat_FromDouble(JS_VALUE_GET_FLOAT64(val));
    case JS_TAG_BIG_INT:
    case JS_TAG_SHORT_BIG_INT:
    {
        const char *str = JS_ToCString(ctx, val);
        if (!str)
            return NULL;
        PyObject *py_val = PyLong_FromString(str, NULL, 10);
        JS_FreeCString(ctx, str);
        return py_val;
    }
    case JS_TAG_STRING:
    {
        const char *str = JS_ToCString(ctx, val);
        if (!str)
            return NULL;
        PyObject *py_str = PyUnicode_FromString(str);
        JS_FreeCString(ctx, str);
        return py_str;
    }
    case JS_TAG_NULL:
        Py_RETURN_NONE;
    case JS_TAG_UNDEFINED:
        Py_RETURN_NONE;
    case JS_TAG_OBJECT:
    {
        PyObject *obj = JS_GetOpaque(val, py_obj_class_id);
        if (obj)
        {
            Py_INCREF(obj);
            return obj;
        }

        if (JS_IsArray(ctx, val))
        {
            JSValue len_val = JS_GetPropertyStr(ctx, val, "length");
            uint32_t len;
            JS_ToUint32(ctx, &len, len_val);
            JS_FreeValue(ctx, len_val);
            PyObject *list = PyList_New(len);
            for (uint32_t i = 0; i < len; i++)
            {
                JSValue item = JS_GetPropertyUint32(ctx, val, i);
                PyList_SetItem(list, i, js_value_to_py(ctx, item));
                JS_FreeValue(ctx, item);
            }
            return list;
        }

        // Return as a dictionary for other objects
        PyObject *dict = PyDict_New();
        JSPropertyEnum *ptab;
        uint32_t plen;
        if (JS_GetOwnPropertyNames(ctx, &ptab, &plen, val, JS_GPN_STRING_MASK | JS_GPN_SYMBOL_MASK) >= 0)
        {
            for (uint32_t i = 0; i < plen; i++)
            {
                JSValue prop_val = JS_GetProperty(ctx, val, ptab[i].atom);
                const char *key = JS_AtomToCString(ctx, ptab[i].atom);
                if (key)
                {
                    PyObject *py_val = js_value_to_py(ctx, prop_val);
                    if (py_val)
                    {
                        PyDict_SetItemString(dict, key, py_val);
                        Py_DECREF(py_val);
                    }
                    JS_FreeCString(ctx, key);
                }
                JS_FreeValue(ctx, prop_val);
                JS_FreeAtom(ctx, ptab[i].atom);
            }
            js_free(ctx, ptab);
        }
        return dict;
    }
    case JS_TAG_EXCEPTION:
    {
        JSValue exc = JS_GetException(ctx);
        JSValue val = JS_GetPropertyStr(ctx, exc, "stack");
        if (JS_IsUndefined(val))
        {
            val = JS_DupValue(ctx, exc);
        }
        const char *str = JS_ToCString(ctx, val);
        if (str)
        {
            PyErr_SetString(PyExc_RuntimeError, str);
            JS_FreeCString(ctx, str);
        }
        else
        {
            PyErr_SetString(PyExc_RuntimeError, "Unknown QuickJS exception");
        }
        JS_FreeValue(ctx, val);
        JS_FreeValue(ctx, exc);
        return NULL;
    }
    default:
        // For now, convert other types to string
        {
            const char *str = JS_ToCString(ctx, val);
            if (!str)
                Py_RETURN_NONE;
            PyObject *py_str = PyUnicode_FromString(str);
            JS_FreeCString(ctx, str);
            return py_str;
        }
    }
}

static JSValue py_to_js_value(JSContext *ctx, PyObject *obj)
{
    if (obj == Py_None)
        return JS_NULL;
    if (PyBool_Check(obj))
        return JS_NewBool(ctx, obj == Py_True);
    if (PyLong_Check(obj))
    {
        int overflow;
        long long v = PyLong_AsLongLongAndOverflow(obj, &overflow);
        if (!overflow)
        {
            if (v >= INT32_MIN && v <= INT32_MAX)
            {
                return JS_NewInt32(ctx, (int32_t)v);
            }
            // Check for safe integer range for Number (53 bits)
            if (v >= -9007199254740991LL && v <= 9007199254740991LL)
            {
                return JS_NewFloat64(ctx, (double)v);
            }
            return JS_NewBigInt64(ctx, v);
        }
        PyErr_Clear();

        // Too big for long long, convert via string to BigInt
        PyObject *py_str = PyObject_Str(obj);
        if (!py_str)
            return JS_EXCEPTION;
        const char *str = PyUnicode_AsUTF8(py_str);

        JSValue global = JS_GetGlobalObject(ctx);
        JSValue bigint_ctor = JS_GetPropertyStr(ctx, global, "BigInt");
        JS_FreeValue(ctx, global);

        JSValue str_val = JS_NewString(ctx, str);
        JSValue result = JS_Call(ctx, bigint_ctor, JS_UNDEFINED, 1, &str_val);

        JS_FreeValue(ctx, str_val);
        JS_FreeValue(ctx, bigint_ctor);
        Py_DECREF(py_str);
        return result;
    }
    if (PyFloat_Check(obj))
        return JS_NewFloat64(ctx, PyFloat_AsDouble(obj));
    if (PyUnicode_Check(obj))
        return JS_NewString(ctx, PyUnicode_AsUTF8(obj));

    if (PyList_Check(obj))
    {
        JSValue arr = JS_NewArray(ctx);
        Py_ssize_t len = PyList_Size(obj);
        for (Py_ssize_t i = 0; i < len; i++)
        {
            PyObject *item = PyList_GetItem(obj, i);
            JS_SetPropertyUint32(ctx, arr, (uint32_t)i, py_to_js_value(ctx, item));
        }
        return arr;
    }

    if (PyDict_Check(obj))
    {
        JSValue js_obj = JS_NewObject(ctx);
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(obj, &pos, &key, &value))
        {
            const char *key_str = PyUnicode_AsUTF8(key);
            if (key_str)
            {
                JS_SetPropertyStr(ctx, js_obj, key_str, py_to_js_value(ctx, value));
            }
        }
        return js_obj;
    }

    if (PyCallable_Check(obj))
    {
        JSValue js_obj = JS_NewObjectClass(ctx, py_obj_class_id);
        Py_INCREF(obj);
        JS_SetOpaque(js_obj, obj);
        JSValue func = JS_NewCFunctionData(ctx, py_callable_handler, 0, 0, 1, &js_obj);
        JS_FreeValue(ctx, js_obj);
        return func;
    }

    // Default: wrap as opaque Python object
    JSValue js_obj = JS_NewObjectClass(ctx, py_obj_class_id);
    Py_INCREF(obj);
    JS_SetOpaque(js_obj, obj);
    return js_obj;
}

static JSValue py_callable_handler(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic, JSValue *func_data)
{
    PyObject *py_func = JS_GetOpaque(func_data[0], py_obj_class_id);
    if (!py_func)
        return JS_EXCEPTION;

    PyGILState_STATE gstate = PyGILState_Ensure();
    PyObject *py_args = PyTuple_New(argc);
    for (int i = 0; i < argc; i++)
    {
        PyObject *arg = js_value_to_py(ctx, argv[i]);
        if (!arg)
        {
            Py_DECREF(py_args);
            PyGILState_Release(gstate);
            return JS_EXCEPTION;
        }
        PyTuple_SetItem(py_args, i, arg);
    }

    PyObject *py_res = PyObject_CallObject(py_func, py_args);
    Py_DECREF(py_args);

    JSValue res;
    if (!py_res)
    {
        PyObject *ptype, *pvalue, *ptraceback;
        PyErr_Fetch(&ptype, &pvalue, &ptraceback);
        const char *msg = "Python error";
        if (pvalue)
        {
            PyObject *pstr = PyObject_Str(pvalue);
            if (pstr)
                msg = PyUnicode_AsUTF8(pstr);
        }
        res = JS_ThrowInternalError(ctx, "%s", msg);
        Py_XDECREF(ptype);
        Py_XDECREF(pvalue);
        Py_XDECREF(ptraceback);
    }
    else
    {
        res = py_to_js_value(ctx, py_res);
        Py_DECREF(py_res);
    }

    PyGILState_Release(gstate);
    return res;
}

static PyObject *Context_Set(Context *self, PyObject *args)
{
    const char *name;
    PyObject *value;
    if (!PyArg_ParseTuple(args, "sO", &name, &value))
        return NULL;

    JSValue js_val = py_to_js_value(self->ctx, value);
    if (JS_IsException(js_val))
    {
        return js_value_to_py(self->ctx, JS_EXCEPTION);
    }
    JSValue global = JS_GetGlobalObject(self->ctx);
    JS_SetPropertyStr(self->ctx, global, name, js_val);
    JS_FreeValue(self->ctx, global);
    Py_RETURN_NONE;
}

static PyObject *Context_Eval(Context *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"code", "filename", NULL};
    const char *code;
    const char *filename = "input.js";
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|s", kwlist, &code, &filename))
    {
        return NULL;
    }

    JSValue val = JS_Eval(self->ctx, code, strlen(code), filename, JS_EVAL_TYPE_GLOBAL);
    PyObject *result = js_value_to_py(self->ctx, val);
    JS_FreeValue(self->ctx, val);
    return result;
}

static PyObject *Context_EvalSync(Context *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"code", "filename", NULL};
    const char *code;
    const char *filename = "input.js";
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|s", kwlist, &code, &filename))
    {
        return NULL;
    }

    JSValue val = JS_Eval(self->ctx, code, strlen(code), filename, JS_EVAL_TYPE_GLOBAL);

    // Run the job queue until empty
    JSContext *ctx1;
    int err;
    while (JS_IsJobPending(self->runtime->rt))
    {
        err = JS_ExecutePendingJob(self->runtime->rt, &ctx1);
        if (err < 0)
        {
            JS_FreeValue(self->ctx, val);
            // js_value_to_py with JS_EXCEPTION will set the Python error
            return js_value_to_py(self->ctx, JS_EXCEPTION);
        }
    }

    PyObject *result = js_value_to_py(self->ctx, val);
    JS_FreeValue(self->ctx, val);
    return result;
}

static PyObject * Context_GetRuntime(Context *self, PyObject *Py_UNUSED(ignored))
{
    Py_INCREF(self->runtime);
    return (PyObject *)self->runtime;
}

static PyObject *Context_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_RuntimeError, "Cannot create Context directly; use Runtime.new_context()");
    return NULL;
}

static void Context_dealloc(Context *self)
{
    if (self->ctx)
        JS_FreeContext(self->ctx);
    Py_XDECREF(self->runtime);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyMethodDef Context_methods[] = {
    {"eval", (PyCFunction)Context_Eval, METH_VARARGS | METH_KEYWORDS, "Evaluate JavaScript code"},
    {"eval_sync", (PyCFunction)Context_EvalSync, METH_VARARGS | METH_KEYWORDS, "Evaluate JavaScript code and run pending jobs"},
    {"set", (PyCFunction)Context_Set, METH_VARARGS, "Set a global value"},
    {"get_runtime", (PyCFunction)Context_GetRuntime, METH_NOARGS, "Get the Runtime associated with this Context"},
    {NULL} /* Sentinel */
};

static PyTypeObject ContextType = {
    PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "_quickjs.Context",
    .tp_doc = "QuickJS Context",
    .tp_basicsize = sizeof(Context),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = Context_new,
    .tp_dealloc = (destructor)Context_dealloc,
    .tp_methods = Context_methods,
};





/*----------------------------------------------------------------------------*/
/* Module definition */

static PyModuleDef quickjsmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_quickjs",
    .m_doc = "QuickJS bindings",
    .m_size = -1,
};

#define DECLARE_TYPE(name)                                         \
    if (PyType_Ready(&name##Type) < 0)                             \
        return NULL;                                               \
    Py_INCREF(&name##Type);                                        \
    if (PyModule_AddObject(m, #name, (PyObject *)&name##Type) < 0) \
    {                                                              \
        Py_DECREF(&name##Type);                                    \
        Py_DECREF(m);                                              \
        return NULL;                                               \
    }

PyMODINIT_FUNC
PyInit__quickjs(void)
{
    /* Initialize the module */
    PyObject *m;
    m = PyModule_Create(&quickjsmodule);
    if (m == NULL)
        return NULL;

    JS_NewClassID(&py_obj_class_id);

    DECLARE_TYPE(Runtime);
    DECLARE_TYPE(Context);

    return m;
}
