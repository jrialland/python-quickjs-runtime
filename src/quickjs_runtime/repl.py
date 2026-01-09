import sys
from . import Runtime
from .require import Require

def main():
    rt = Runtime()
    ctx = rt.new_context()
    Require(ctx, ".")
    
    print("QuickJS REPL")
    eof_key = "Ctrl+Z" if sys.platform == "win32" else "Ctrl+D"
    print(f"Type 'exit()' or {eof_key} to exit")
    
    while True:
        try:
            line = input("> ")
            if line == "exit()":
                break
            try:
                result = ctx.eval(line)
                if result is not None:
                    print(result)
            except RuntimeError as e:
                print(f"Error: {e}")
        except EOFError:
            print()
            break
        except KeyboardInterrupt:
            print("\nKeyboardInterrupt")
            continue

if __name__ == "__main__":
    main()
