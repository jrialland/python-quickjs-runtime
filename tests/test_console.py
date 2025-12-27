import pytest
from quickjs_runtime import Context

def test_console_log(capsys):
    ctx = Context()
    ctx.eval("console.log('hello', 'world')")
    captured = capsys.readouterr()
    assert captured.out == "hello world\n"

def test_console_error(capsys):
    ctx = Context()
    ctx.eval("console.error('oops')")
    captured = capsys.readouterr()
    assert captured.err == "oops\n"

def test_console_cleanup():
    ctx = Context()
    # Check that temporary globals are removed
    with pytest.raises(RuntimeError, match="ReferenceError"):
        ctx.eval("__py_log")
