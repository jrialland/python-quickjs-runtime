import pytest
from quickjs_runtime import Context

def test_eval_basic():
    ctx = Context()
    assert ctx.eval("1 + 1") == 2

def test_eval_string():
    ctx = Context()
    assert ctx.eval("'hello' + ' ' + 'world'") == "hello world"

def test_eval_error():
    ctx = Context()
    with pytest.raises(RuntimeError):
        ctx.eval("throw new Error('oops')")

def test_context_persistence():
    ctx = Context()
    ctx.eval("var x = 10;")
    assert ctx.eval("x") == 10

def test_set_variable():
    ctx = Context()
    ctx.set("myVar", 42)
    assert ctx.eval("myVar") == 42
    
    ctx.set("myStr", "hello")
    assert ctx.eval("myStr") == "hello"

def test_python_callback():
    ctx = Context()
    
    def add(a, b):
        return a + b
    
    ctx.set("add", add)
    assert ctx.eval("add(10, 20)") == 30

def test_python_callback_error():
    ctx = Context()
    
    def fail():
        raise ValueError("Something went wrong")
    
    ctx.set("fail", fail)
    with pytest.raises(RuntimeError, match="Python Exception: Something went wrong"):
        ctx.eval("fail()")
