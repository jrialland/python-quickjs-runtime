import sys
from . import Runtime
from .require import Require


def run_interactive(ctx):
    print("QuickJS REPL")
    eof_key = "Ctrl+Z" if sys.platform == "win32" else "Ctrl+D"
    print(f"Type 'exit()' or {eof_key} to exit")

    while True:
        try:
            line = input("> ")
            if line == "exit()":
                break
            try:
                result = ctx.eval_sync(line)
                if result is not None:
                    print(result)
            except RuntimeError as e:
                print(f"Error: {e}")
        except EOFError:
            print()
            break
        except KeyboardInterrupt:
            print()
            break


def run_non_interactive(ctx):
    code = sys.stdin.read()
    if code.strip():
        try:
            result = ctx.eval_sync(code)
            if result is not None:
                print(result)
        except RuntimeError as e:
            print(f"Error: {e}", file=sys.stderr)
            sys.exit(1)


def main():
    rt = Runtime()
    ctx = rt.new_context()
    Require(ctx, ".")

    if sys.stdin.isatty():
        run_interactive(ctx)
    else:
        run_non_interactive(ctx)


if __name__ == "__main__":
    main()
