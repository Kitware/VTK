"""Test the syntax of the .pyi files for all modules in a package.

The pyi files provide type information for each wrapped VTK module,
see PEP 484 for more information on type annotations in Python.

This test verifies the syntax of the .pyi files, since they cannot
be used unless the syntax is correct.
"""

import os
import sys
import ast
import traceback

# flag to parse type comments (Python 3.8 and later)
flags = 0
if sys.hexversion >= 0x3080000:
    flags = ast.PyCF_TYPE_COMMENTS

def TestCompile(filename):
    try:
        with open(filename, 'r') as f:
            compile(f.read(), filename, 'exec', flags)
        rval = 0
    except SyntaxError:
        traceback.print_exc(0)
        rval = 1
    return rval

def main(argv=sys.argv):
    if len(sys.argv) < 2:
        sys.stderr.write("Package directory not specified\n")
        sys.stderr.write("Usage: %s <directory> [modules]\n" % sys.argv[0])
        sys.exit(1)

    packagedir = sys.argv[1]
    modules = sys.argv[2:]

    if len(modules) == 0:
        sys.stderr.write("No modules provided for testing!\n");
        sys.exit(1)

    rval = 0
    for m in modules:
        module_pyi = os.path.join(packagedir, m + ".pyi")
        rval = rval or TestCompile(module_pyi)

    return rval

if __name__ == '__main__':
    sys.exit(main())
