# python-quickjs-runtime

A lightweight Python binding for the [QuickJS](https://bellard.org/quickjs/) Javascript Engine.

This library allows you to execute JavaScript code within your Python applications, providing seamless interoperability between Python and JavaScript.

## Features

*   **Evaluate JavaScript**: Run JS code strings directly from Python.
*   **ES2023 Support**: Support for modern JS features including `async/await`.
*   **Data Exchange**: Pass `int`, `float`, `str`, `bool`, `list`, `dict`, and `BigInt` between Python and JS.
*   **Python Callbacks**: Call Python functions directly from JavaScript.
*   **REPL**: Includes a simple interactive shell for testing JS snippets.
*   **Easy Build**: Uses `zigcc-build` for reliable cross-platform compilation.

## Installation

You can install the package from the source:

```bash
pip install .
```

## Usage

### Basic Evaluation

```python
from quickjs_runtime import Runtime

rt = Runtime()
ctx = rt.new_context()

# Evaluate simple expressions
result = ctx.eval("1 + 2")
print(result)  # 3

# Evaluate strings
greeting = ctx.eval("'Hello ' + 'World'")
print(greeting)  # "Hello World"
```

### Interoperability

You can set global variables in the JavaScript context and retrieve results.

```python
from quickjs_runtime import Runtime

rt = Runtime()
ctx = rt.new_context()

# Set variables
ctx.set("x", 10)
ctx.set("y", 20)

# Use them in JS
result = ctx.eval("x * y")
print(result)  # 200
```

### Calling Python from JavaScript

You can pass Python functions to the JavaScript context.

```python
from quickjs_runtime import Runtime

def add(a, b):
    return a + b

rt = Runtime()
ctx = rt.new_context()
ctx.set("py_add", add)

# Call the Python function from JS
result = ctx.eval("py_add(5, 7)")
print(result)  # 12
```

### Custom Filenames and Stack Traces

You can provide a filename to `eval` for better error messages.

```python
from quickjs_runtime import Runtime

rt = Runtime()
ctx = rt.new_context()

try:
    ctx.eval("throw new Error('oops')", filename="my_script.js")
except RuntimeError as e:
    print(e)  # error message containing 'my_script.js'
```

### Async/Await Support

Wait for Promises to settle using `eval_sync`.

```python
from quickjs_runtime import Runtime

rt = Runtime()
ctx = rt.new_context()

script = """
async function fetchData() {
    return "done";
}
fetchData();
"""

# eval_sync will run the job queue until all promises are settled
ctx.eval_sync(script) 
```

## REPL

The package includes a simple REPL (Read-Eval-Print Loop).

```bash
python -m quickjs_runtime
```

## Development

This project uses `zigcc-build` to compile the C extensions. This ensures a consistent build environment without needing to install system-wide C compilers.

To build and install locally:

```bash
python -m build
pip install dist/*.whl
```

To run tests:

```bash
pytest
```
