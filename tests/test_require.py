import os
import pytest
from quickjs_runtime import Context
from quickjs_runtime.require import Require

def test_require_simple(tmp_path):
    # Create a dummy module in a temporary directory
    module_file = tmp_path / "my_module.js"
    module_file.write_text("exports.add = function(a, b) { return a + b; };", encoding="utf-8")
    
    ctx = Context()
    # Initialize require with the temp directory as base
    Require(ctx, str(tmp_path))
    
    res = ctx.eval("var m = require('./my_module'); m.add(10, 20);")
    assert res == 30

def test_require_nested(tmp_path):
    # Create a nested structure
    # main.js -> utils/math.js
    
    (tmp_path / "utils").mkdir()
    (tmp_path / "utils" / "math.js").write_text("exports.square = function(x) { return x * x; };", encoding="utf-8")
    (tmp_path / "main.js").write_text("""
        var math = require('./utils/math');
        exports.calc = function(x) { return math.square(x) + 1; };
    """, encoding="utf-8")
    
    ctx = Context()
    Require(ctx, str(tmp_path))
    
    res = ctx.eval("var main = require('./main'); main.calc(5);")
    assert res == 26

def test_require_not_found():
    ctx = Context()
    Require(ctx, ".")
    with pytest.raises(RuntimeError, match="Module not found"):
        ctx.eval("require('./non_existent_module')")

def test_require_cache(tmp_path):
    module_file = tmp_path / "counter.js"
    module_file.write_text("""
        if (!globalThis.counter) globalThis.counter = 0;
        globalThis.counter++;
        exports.get = function() { return globalThis.counter; };
    """, encoding="utf-8")
    
    ctx = Context()
    Require(ctx, str(tmp_path))
    
    # First require should increment counter
    ctx.eval("var c1 = require('./counter');")
    assert ctx.eval("c1.get()") == 1
    
    # Second require should use cache, so counter should NOT increment
    ctx.eval("var c2 = require('./counter');")
    assert ctx.eval("c2.get()") == 1
