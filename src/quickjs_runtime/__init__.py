from _quickjs import Context as _Context
import sys

class Context:
    """
    A QuickJS Context wrapper.
    """
    def __init__(self, with_console: bool = True):
        self._ctx = _Context()
        if with_console:
            self._setup_console()

    def _setup_console(self):
        def log(*args):
            print(*args)
        
        def error(*args):
            print(*args, file=sys.stderr)

        self.set("__py_log", log)
        self.set("__py_error", error)
        
        self.eval("""
            (function() {
                var log = __py_log;
                var error = __py_error;
                globalThis.console = {
                    log: function(...args) { log(...args); },
                    error: function(...args) { error(...args); },
                    warn: function(...args) { error(...args); },
                    info: function(...args) { log(...args); }
                };
            })();
        """)
        # Clean up globals
        self.eval("delete globalThis.__py_log; delete globalThis.__py_error;")

    def eval(self, code: str, filename: str = "<input>"):
        """
        Evaluate JavaScript code and return the result.
        
        :param code: The JavaScript code to evaluate.
        :param filename: The filename to use for stack traces (optional).
        :return: The result of the evaluation (int, float, str, bool, None).
        :raises RuntimeError: If the JavaScript code throws an exception.
        """
        return self._ctx.eval(code, filename)

    def eval_file(self, path: str):
        """
        Evaluate a JavaScript file.
        
        :param path: The path to the file to evaluate.
        :return: The result of the evaluation.
        """
        with open(path, "r", encoding="utf-8") as f:
            code = f.read()
        return self.eval(code, filename=path)

    def set(self, name: str, value):
        """
        Set a global variable in the JavaScript context.
        
        :param name: The name of the variable.
        :param value: The value to set (int, float, str, bool, None, or callable).
        """
        self._ctx.set(name, value)
