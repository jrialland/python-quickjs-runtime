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

def configure(config: ZigCcConfig):
    print("Configuring QuickJS build...")
    
    qjs_root = "quickjs"
    
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
    config["defines"].append('CONFIG_VERSION="2025-09-13"')
    config["defines"].append("CONFIG_BIGNUM")
 