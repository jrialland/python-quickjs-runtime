from quickjs_runtime import Runtime


def test_runtime_creation():
    rt = Runtime()
    assert isinstance(rt, Runtime)


def test_set_memory_limit():
    rt = Runtime()
    rt.set_memory_limit(1024 * 1024)  # 1 MB
    # Assuming there's a method to get memory limit for verification
    # assert rt.get_memory_limit() == 1024 * 1024


def test_set_gc_threshold():
    rt = Runtime()
    rt.set_gc_threshold(512 * 1024)  # 512 KB
    # Assuming there's a method to get GC threshold for verification
    # assert rt.get_gc_threshold() == 512 * 1024


def test_set_max_stack_size():
    rt = Runtime()
    rt.set_max_stack_size(64 * 1024)  # 64 KB
    # Assuming there's a method to get max stack size for verification
    # assert rt.get_max_stack_size() == 64 * 1024


def test_update_stack_top():
    rt = Runtime()
    rt.update_stack_top()
    # No return value to assert; just ensure no exceptions are raised


def test_run_gc():
    rt = Runtime()
    rt.run_gc()
    # No return value to assert; just ensure no exceptions are raised

def test_new_context():
    rt = Runtime()
    ctx = rt.new_context()
    print(type(ctx))
    assert ctx is not None


def test_context_eval():
    rt = Runtime()
    ctx = rt.new_context()

    # Test number
    assert ctx.eval("1 + 1") == 2

    # Test string
    assert ctx.eval("'hello' + ' world'") == "hello world"

    # Test boolean
    assert ctx.eval("1 == 1") is True
    assert ctx.eval("1 == 0") is False

    # Test null/undefined
    assert ctx.eval("null") is None
    assert ctx.eval("undefined") is None


def test_context_eval_exception():
    import pytest
    rt = Runtime()
    ctx = rt.new_context()
    with pytest.raises(RuntimeError):
        ctx.eval("throw new Error('test error')")


def test_context_set():
    rt = Runtime()
    ctx = rt.new_context()
    ctx.set("a", 10)
    ctx.set("b", 20)
    assert ctx.eval("a + b") == 30


def test_context_call_python():
    rt = Runtime()
    ctx = rt.new_context()

    def add(x, y):
        return x + y

    ctx.set("py_add", add)
    assert ctx.eval("py_add(10, 20)") == 30


def test_context_set_object():
    rt = Runtime()
    ctx = rt.new_context()

    class MyObj:
        def __init__(self, val):
            self.val = val

    obj = MyObj(42)
    ctx.set("o", obj)
    # This should return the same object back
    assert ctx.eval("o") is obj


def test_context_list_dict():
    rt = Runtime()
    ctx = rt.new_context()

    # Python list to JS
    ctx.set("l", [1, 2, 3])
    assert ctx.eval("l.length") == 3
    assert ctx.eval("l[0] + l[1] + l[2]") == 6

    # JS Array back to Python
    res = ctx.eval("[10, 20, 30]")
    assert isinstance(res, list)
    assert res == [10, 20, 30]

    # Python dict to JS
    ctx.set("d", {"a": 1, "b": 2})
    assert ctx.eval("d.a + d.b") == 3

    # JS Object back to Python
    res = ctx.eval("({x: 1, y: 2})")
    assert isinstance(res, dict)
    assert res == {"x": 1, "y": 2}


def test_nested_collections():
    rt = Runtime()
    ctx = rt.new_context()

    nested = {
        "list": [1, {"x": 2}],
        "val": 42
    }
    ctx.set("n", nested)
    assert ctx.eval("n.list[0]") == 1
    assert ctx.eval("n.list[1].x") == 2
    assert ctx.eval("n.val") == 42

    res = ctx.eval("({a: [1, 2], b: {c: 3}})")
    assert res == {"a": [1, 2], "b": {"c": 3}}


def test_eval_sync():
    rt = Runtime()
    ctx = rt.new_context()

    # Test resolving a promise
    script = """
    var out = 0;
    async function test() {
        out = await Promise.resolve(100);
    }
    test();
    out;
    """
    # eval() will return 0 because it doesn't wait for the promise
    assert ctx.eval(script) == 0

    # eval_sync() will wait for the job queue to be empty
    script = """
    var out_async = 0;
    async function test_async() {
        out_async = await Promise.resolve(200);
    }
    test_async();
    out_async;
    """
    # Note: the return value of the script is still eval'd before the loop, 
    # but we want to check if the side effect (out_async = 200) happened.
    ctx.eval_sync(script)
    assert ctx.eval("out_async") == 200


def test_eval_filename():
    rt = Runtime()
    ctx = rt.new_context()

    # Verify positional filename
    try:
        ctx.eval("syntax error", "custom_pos.js")
    except RuntimeError as e:
        assert "custom_pos.js" in str(e)

    # Verify keyword filename
    try:
        ctx.eval(code="syntax error", filename="custom_kw.js")
    except RuntimeError as e:
        assert "custom_kw.js" in str(e)

    # Verify eval_sync keyword filename
    try:
        ctx.eval_sync(code="syntax error", filename="custom_sync_kw.js")
    except RuntimeError as e:
        assert "custom_sync_kw.js" in str(e)

def test_runtime_from_context():
    rt = Runtime()
    ctx = rt.new_context()
    rt_from_ctx = ctx.get_runtime()
    assert rt_from_ctx is rt

def test_bigint_handling():
    rt = Runtime()
    ctx = rt.new_context()

    # Test setting and getting a bigint
    big_value = 1234567890123456789012345678901234567890
    ctx.set("big", big_value)
    assert ctx.eval("typeof big") == "bigint"
    result = ctx.eval("big + 1n")
    assert result == big_value + 1

    # Test evaluating a bigint literal
    result = ctx.eval("1234567890123456789012345678901234567890n + 2n")
    assert result == big_value + 2
    assert isinstance(result, int)

    # Test small bigint (fits in 64-bit)
    small_big = 9007199254740992 # MAX_SAFE_INTEGER + 1
    ctx.set("sb", small_big)
    assert ctx.eval("typeof sb") == "bigint"
    assert ctx.eval("sb") == small_big
    assert isinstance(ctx.eval("sb"), int)