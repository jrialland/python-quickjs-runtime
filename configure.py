"""
This is the build configuration for zigcc-build to compile QuickJS as a Python extension module.
"""

import os
from typing import List, TypedDict

class ZigCcConfig(TypedDict):
    sources: List[str]
    include_dirs: List[str]
    defines: List[str]
    library_dirs: List[str]
    libraries: List[str]
    module_name: str

QJS_ROOT = "quickjs"

REQUIRED_QJS_FILES = [
    "quickjs.c",
    "quickjs.h",
    "libregexp.c",
    "libunicode.c",
    "cutils.c",
    "quickjs-libc.c",
    "dtoa.c",
    "VERSION",
]

def _check_qjs_sources():
    missing = [
        name for name in REQUIRED_QJS_FILES
        if not os.path.isfile(os.path.join(QJS_ROOT, name))
    ]
    if missing:
        raise RuntimeError(
            "QuickJS sources are missing from '{}': {}. "
            "The 'quickjs' git submodule is probably not initialized. Run:\n"
            "    git submodule update --init --recursive".format(
                QJS_ROOT, ", ".join(missing)
            )
        )

def _qjs_version():
    with open(os.path.join(QJS_ROOT, "VERSION"), encoding="utf-8") as f:
        return f.read().strip()

def configure(config: ZigCcConfig):
    print("Configuring QuickJS build...")

    _check_qjs_sources()

    qjs_root = QJS_ROOT
    
    # Core QuickJS sources
    qjs_sources = [
        "libregexp.c",
        "libunicode.c",
        "cutils.c",
        "quickjs-libc.c",
        "dtoa.c",
    ]
    
    # Use wrapper for quickjs.c to ensure stddef.h is included
    config["sources"].append("c-sources/quickjs_wrapper.c")
    
    for src in qjs_sources:
        config["sources"].append(os.path.join(qjs_root, src))
    
    # Add binding source
    config["sources"].append("c-sources/binding.c")
    
    # Set module name to _quickjs (the extension)
    config["module_name"] = "_quickjs"
    
    config["include_dirs"].append(qjs_root)
    
    # Standard QuickJS defines
    config["defines"].append("_GNU_SOURCE")
    config["defines"].append('CONFIG_VERSION="{}"'.format(_qjs_version()))
    config["defines"].append("CONFIG_BIGNUM")
