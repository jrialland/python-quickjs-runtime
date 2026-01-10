from _quickjs import Runtime as _Runtime
import sys

"""
    {"set_runtime_info", (PyCFunction)Runtime_SetRuntimeInfo, METH_O, "Set runtime info"},
    {"set_memory_limit", (PyCFunction)Runtime_SetMemoryLimit, METH_O, "Set memory limit"},
    {"set_gc_threshold", (PyCFunction)Runtime_SetGCThreshold, METH_O, "Set GC threshold"},
    {"set_max_stack_size", (PyCFunction)Runtime_SetMaxStackSize, METH_O, "Set max stack size"},
    {"update_stack_top", (PyCFunction)Runtime_UpdateStackTop, METH_NOARGS, "Update stack top"},
    {"run_gc", (PyCFunction)Runtime_RunGC, METH_NOARGS, "Run garbage collector"},
    {"new_context", (PyCFunction)Runtime_NewContext, METH_NOARGS, "Create a new QuickJS Context"},

"""
from abc import ABC, abstractmethod
from typing import override

class IRuntime(ABC):

    @abstractmethod
    def set_runtime_info(self, info: str) -> None:
        ...
    
    @abstractmethod
    def set_memory_limit(self, limit: int) -> None:
        ...
    
    @abstractmethod
    def set_gc_threshold(self, threshold: int) -> None:
        ...
    
    @abstractmethod
    def set_max_stack_size(self, size: int) -> None:
        ...
    
    @abstractmethod
    def update_stack_top(self) -> None:
        ...
    
    @abstractmethod
    def run_gc(self) -> None:
        ...

    @abstractmethod
    def new_context(self) -> "IContext":
        ...

class Context(ABC):

    @abstractmethod
    def eval(self, code: str, filename: str = "input.js") -> any:
        ...

    @abstractmethod
    def eval_sync(self, code: str, filename: str = "input.js") -> any:
        ...

    @abstractmethod
    def set(self, name: str, value: any) -> None:
        ...

    @abstractmethod
    def get_runtime(self) -> IRuntime:
        ...


class Runtime(IRuntime, _Runtime):

    def __init__(self) -> None:
        _Runtime.__init__(self)

    @override
    def set_runtime_info(self, info: str) -> None:
        return _Runtime.set_runtime_info(self, info)
    
    @override
    def set_memory_limit(self, limit: int) -> None:
        return _Runtime.set_memory_limit(self, limit)
    
    @override
    def set_gc_threshold(self, threshold: int) -> None:
        return _Runtime.set_gc_threshold(self, threshold)
    
    @override
    def set_max_stack_size(self, size: int) -> None:
        return _Runtime.set_max_stack_size(self, size)
    
    @override
    def update_stack_top(self) -> None:
        return _Runtime.update_stack_top(self)
    
    @override
    def run_gc(self) -> None:
        return _Runtime.run_gc(self)
    
    @override
    def new_context(self) -> Context:
        ctx = _Runtime.new_context(self)
        
        # Setup console object
        console = {
            "log": lambda *args: print(*args),
            "info": lambda *args: print(*args),
            "warn": lambda *args: print(*args),
            "error": lambda *args: print(*args, file=sys.stderr),
        }
        ctx.set("console", console)
        
        return ctx


__all__ = ["Runtime", "Context"]