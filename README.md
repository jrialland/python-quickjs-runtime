# python-quickjs-runtime

A lightweight, high-performance Python binding for the [QuickJS](https://bellard.org/quickjs/) Javascript Engine.

This library allows you to execute JavaScript code within your Python applications, providing seamless interoperability between Python and JavaScript.

## Features

*   **Evaluate JavaScript**: Run JS code strings or files directly from Python.
*   **Data Exchange**: Pass `int`, `float`, `str`, `bool`, and `None` between Python and JS.
*   **Python Callbacks**: Call Python functions directly from JavaScript.
*   **Console Support**: Built-in `console.log`, `console.error`, etc., mapped to Python's stdout/stderr.
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
from quickjs_runtime import Context

ctx = Context()

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
ctx = Context()

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
def add(a, b):
    return a + b

ctx = Context()
ctx.set("py_add", add)

# Call the Python function from JS
result = ctx.eval("py_add(5, 7)")
print(result)  # 12
```

### Evaluating Files

```python
ctx = Context()
# Assuming 'script.js' exists
result = ctx.eval_file("script.js")
```

### Console Output

The `console` object is available in the JS context and redirects to Python's standard output/error.

```python
ctx = Context()
ctx.eval("console.log('This goes to stdout')")
ctx.eval("console.error('This goes to stderr')")
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
