import os
# from . import Context  <-- Removed to avoid circular import

class Require:
    """
    Implements a CommonJS-style 'require' mechanism for QuickJS.
    """
    def __init__(self, ctx, base_path: str = "."):
        """
        Initialize the Require mechanism.
        
        :param ctx: The Context instance to attach to.
        :param base_path: The root path for resolving modules (default: current working directory).
        """
        self.ctx = ctx
        self.base_path = os.path.abspath(base_path)
        self._setup()

    def _setup(self):
        def _read_file(path):
            try:
                # Normalize path for OS
                path = os.path.normpath(path)
                with open(path, "r", encoding="utf-8") as f:
                    return f.read()
            except Exception as e:
                raise RuntimeError(f"Cannot read file {path}: {e}")

        def _resolve(current_dir, module_name):
            # current_dir is where the calling module resides (or "." for root)
            # If current_dir is ".", use base_path
            if current_dir == ".":
                start_dir = self.base_path
            else:
                start_dir = os.path.normpath(current_dir)

            if module_name.startswith("."):
                target = os.path.join(start_dir, module_name)
            else:
                # Absolute or relative to base_path (simple node_modules simulation could go here)
                target = os.path.join(self.base_path, module_name)
            
            # Add .js if missing
            if not target.endswith(".js"):
                target += ".js"
            
            target = os.path.abspath(target)
            if not os.path.exists(target):
                 raise RuntimeError(f"Module not found: {module_name} (at {target})")
            
            # Return with forward slashes for JS consistency
            return target.replace(os.sep, "/")

        self.ctx.set("__py_require_read", _read_file)
        self.ctx.set("__py_require_resolve", _resolve)
        
        # Init cache if not exists
        self.ctx.eval("if (!globalThis.__require_cache) globalThis.__require_cache = {};")

        # Define require factory and install root require
        loader_script = """
        (function() {
            var read = __py_require_read;
            var resolve = __py_require_resolve;
            var cache = globalThis.__require_cache;

            function make_require(current_dir) {
                return function require(module_name) {
                    // console.log("Resolving", module_name, "from", current_dir);
                    var full_path = resolve(current_dir, module_name);
                    // console.log("Resolved to", full_path);
                    
                    if (cache[full_path]) {
                        return cache[full_path].exports;
                    }

                    var content = read(full_path);
                    
                    // CommonJS wrapper
                    // We wrap in a function to provide scope
                    var wrapper_code = "(function(exports, require, module, __filename, __dirname) { " + content + "\\n})";
                    
                    // console.log("Loading:", full_path);
                    
                    var compiled_wrapper;
                    try {
                        compiled_wrapper = eval(wrapper_code);
                    } catch (e) {
                        throw new Error("Syntax error in " + full_path + ": " + e.message);
                    }
                    
                    var module = { exports: {} };
                    cache[full_path] = module;
                    
                    var new_dir = full_path.substring(0, full_path.lastIndexOf('/'));
                    
                    try {
                        compiled_wrapper(module.exports, make_require(new_dir), module, full_path, new_dir);
                    } catch (e) {
                        // Clean up cache on error? Maybe not, to avoid infinite loops if cyclic?
                        // But usually we want to fail hard.
                        throw e;
                    }
                    
                    return module.exports;
                };
            }

            // Install root require if not present
            if (!globalThis.require) {
                globalThis.require = make_require(".");
            }
        })();
        """
        self.ctx.eval(loader_script)
