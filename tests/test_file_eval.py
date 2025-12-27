import pytest
from quickjs_runtime import Context
import os

def test_eval_file(tmp_path):
    js_file = tmp_path / "test.js"
    js_file.write_text("var x = 100; x * 2;", encoding="utf-8")
    
    ctx = Context()
    result = ctx.eval_file(str(js_file))
    assert result == 200

def test_eval_file_error(tmp_path):
    js_file = tmp_path / "error.js"
    js_file.write_text("throw new Error('file error');", encoding="utf-8")
    
    ctx = Context()
    with pytest.raises(RuntimeError, match="file error"):
        ctx.eval_file(str(js_file))
