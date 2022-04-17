#!/usr/bin/env python

"""
This program will generate .pyi files for all the VTK modules
in the "vtkmodules" package (or whichever package you specify).
These files are used for type checking and autocompletion in
some Python IDEs.

The VTK modules must be in Python's path when you run this script.
Options are as follows:

 -p PACKAGE    The package to generate .pyi files for [vtkmodules]
 -o OUTPUT     The output directory [default is the package directory]
 -e EXT        The file suffix [.pyi]
 -h HELP

With no arguments, the script runs with the defaults (the .pyi files
are put inside the existing vtkmodules package).  This is equivalent
to the following:

    python -m vtkmodules.generate_pyi -p vtkmodules

To put the pyi files somewhere else, perhaps with a different suffix:

    python -m vtkmodules.generate_pyi -o /path/to/vtkmodules -e .pyi

To generate pyi files for just one or two modules:

    python -m vtkmodules.generate_pyi -p vtkmodules vtkCommonCore vtkCommonDataModel

To generate pyi files for your own modules in your own package:

    python -m vtkmodules.generate_pyi -p mypackage mymodule [mymodule2 ...]

"""

from vtkmodules.vtkCommonCore import vtkObject, vtkSOADataArrayTemplate

import sys
import os
import re
import ast
import argparse
import builtins
import inspect
import importlib.util


# ==== For type inspection ====

# list expected non-vtk type names
types = set()
for m,o in builtins.__dict__.items():
    if isinstance(o, type):
        types.add(m)
for m in ['Any', 'Buffer', 'Callback', 'None', 'Pointer', 'Template', 'Union']:
    types.add(m)

# basic type checking methods
ismethod = inspect.isroutine
isclass = inspect.isclass

# VTK methods have a special type
vtkmethod = type(vtkObject.IsA)
template = type(vtkSOADataArrayTemplate)

def isvtkmethod(m):
    """Check for VTK's custom method descriptor"""
    return (type(m) == vtkmethod)

def isnamespace(m):
    """Check for namespaces within a module"""
    # until vtkmodules.vtkCommonCore.namespace is directly accessible
    return (str(type(m)) == "<class 'vtkmodules.vtkCommonCore.namespace'>")

def isenum(m):
   """Check for enums (currently derived from int)"""
   return (isclass(m) and issubclass(m, int))

def typename(o):
    """Generate a typename that can be used for annotation."""
    if o is None:
        return "None"
    elif type(o) == template:
        return "Template"
    else:
        return type(o).__name__

def typename_forward(o):
    """Generate a typename, or if necessary, a forward reference."""
    name = typename(o)
    if name not in types:
        # do forward reference by adding quotes
        name = '\'' + name + '\''
    return name


# ==== For the topological sort ====

class Graph:
    """A graph for topological sorting."""
    def __init__(self):
        self.nodes = {}
    def __getitem__(self, name):
        return self.nodes[name]
    def __setitem__(self, name, node):
        self.nodes[name] = node

class Node:
    """A node for the graph."""
    def __init__(self, o, d):
        self.obj = o
        self.deps = d

def build_graph(d):
    """Build a graph from a module's dictionary."""
    graph = Graph()
    items = sorted(d.items())
    for m,o in items:
        if isclass(o):
            if m == o.__name__:
                # a class definition
                bases = [b.__name__ for b in o.__bases__]
                graph[m] = Node(o, bases)
            else:
                # a class alias
                graph[m] = Node(o, [o.__name__])
        elif ismethod(o):
            graph[m] = Node(o, [])
        else:
            graph[m] = Node(o, [typename(o)])
    return graph

def sorted_graph_helper(graph, m, visited, items):
    """Helper for topological sorting."""
    visited.add(m)
    try:
        node = graph[m]
    except KeyError:
        return
    for dep in node.deps:
        if dep not in visited:
            sorted_graph_helper(graph, dep, visited, items)
    items.append((m, node.obj))

def sorted_graph(graph):
    """Sort a graph and return the sorted items."""
    items = []
    visited = set()
    for m in graph.nodes:
        if m not in visited:
             sorted_graph_helper(graph, m, visited, items)
    return items

def topologically_sorted_items(d):
    """Return the items from a module's dictionary, topologically sorted."""
    return sorted_graph(build_graph(d))


# ==== For parsing docstrings ====

# regular expressions for parsing
string = re.compile(r"""("([^\\"]|\\.)*"|'([^\\']|\\.)*')""")
identifier = re.compile(r"""[ \t]*([A-Za-z_]([A-Za-z0-9_]|[.][A-Za-z_])*)""")
indent = re.compile(r"[ \t]+\S")
has_self = re.compile(r"[(]self[,)]")

# important characters for rapidly parsing code
keychar = re.compile(r"[\'\"{}\[\]()>:\n]")

def parse_error(message, text, begin, pos):
    """Print a parse error, syntax or otherwise.
    """
    end = text.find('\n', pos)
    if end == -1:
        end = len(text)
    sys.stderr.write("Error: " + message + ":\n")
    sys.stderr.write(text[begin:end] + "\n");
    sys.stderr.write('-' * (pos - begin) + "^\n")

def push_signature(o, l, signature):
    """Process a method signature and add it to the list.
    """
    signature = re.sub(r"\s+", " ", signature)
    if signature.startswith('C++:'):
        # if C++ method is static, mark Python signature static
        if isvtkmethod(o) and signature.find(" static ") != -1 and len(l) > 0:
            if not l[-1].startswith("@staticmethod"):
                l[-1] = "@staticmethod\n" + l[-1]
    elif signature.startswith(o.__name__ + "("):
        if isvtkmethod(o) and not has_self.search(signature):
            if not signature.startswith("@staticmethod"):
                signature = "@staticmethod\n" + signature
        l.append(signature)

def get_signatures(o):
    """Return a list of method signatures found in the docstring.
    """
    doc = o.__doc__
    signatures = [] # output method signatures
    if doc is None:
        return signatures

    # variables used for parsing the docstrings
    begin = 0 # beginning of current signature
    pos = 0 # current position in docstring
    delim_stack = [] # keep track of bracket depth

    # loop through docstring using longest strides possible
    # (this will go line-by-line or until first ( ) { } [ ] " ' : >)
    while pos < len(doc):
        # look for the next "character of insterest" in docstring
        match = keychar.search(doc, pos)
        # did we find a match before the end of docstring?
        if match:
            # get new position
            pos,end = match.span()
            # take different action, depending on char
            c = match.group()
            if c in '\"\'':
                # skip over a string literal
                m = string.match(doc, pos)
                if m:
                    pos,end = m.span()
                else:
                    parse_error("Unterminated string", doc, begin, pos)
                    break
            elif c in '{[(':
                # descend into a bracketed expression (push stack)
                delim_stack.append({'{':'}','[':']','(':')'}[c])
            elif c in '}])':
                # ascend out of a bracketed expression (pop stack)
                if not delim_stack or c != delim_stack.pop():
                    parse_error("Unmatched bracket", doc, begin, pos)
                    break
            elif c == ':' or (c == '>' and doc[pos-1] == '-'):
                # what follows is a type
                m = identifier.match(doc, pos+1)
                if m:
                    pos,end = m.span(1)
                    name = m.group(1)
                    if name not in types:
                        # quote the type
                        doc = doc[0:pos] + ('\'' + name + '\'') + doc[end:]
                        end += 2
            elif c == '\n' and not (delim_stack or indent.match(doc, end)):
                # a newline not followed by an indent marks end of signature,
                # except for within brackets
                signature = doc[begin:pos].strip()
                if signature and signature not in signatures:
                    push_signature(o, signatures, signature)
                    begin = end
                else:
                    # blank line means no more signatures in docstring
                    break
        else:
            # reached the end of the docstring
            end = len(doc)
            if not delim_stack:
                signature = doc[begin:pos].strip()
                if signature and signature not in signatures:
                    push_signature(o, signatures, signature)
            else:
                parse_error("Unmatched bracket", doc, begin, pos)
                break

        # advance position within docstring and return to head of loop
        pos = end

    return signatures

def get_constructors(c):
    """Get constructors from the class documentation.
    """
    constructors = []
    name = c.__name__
    doc = c.__doc__

    if not doc or not doc.startswith(name + "("):
        return constructors
    signatures = get_signatures(c)
    for signature in signatures:
        if signature.startswith(name + "("):
            signature = re.sub("-> \'?" + name + "\'?", "-> None", signature)
            if signature.startswith(name + "()"):
                constructors.append(re.sub(name + r"\(", "__init__(self", signature, 1))
            else:
                constructors.append(re.sub(name + r"\(", "__init__(self, ", signature, 1))
    return constructors

def add_indent(s, indent):
    """Add the given indent before every line in the string.
    """
    return indent + re.sub(r"\n(?=([^\n]))", "\n" + indent, s)

def make_def(s, indent):
    """Generate a method definition stub from the signature and an indent.
    The indent is a string (tabs or spaces).
    """
    pos = 0
    out = ""
    while pos < len(s) and s[pos] == '@':
        end = s.find('\n', pos) + 1
        if end == 0:
            end = len(s)
        out += indent
        out += s[pos:end]
        pos = end
    if pos < len(s):
        out += indent
        out += "def "
        out += s[pos:]
        out += ": ..."
    return out

def namespace_pyi(c, mod):
    """Fake a namespace by creating a dummy class.
    """
    base = "namespace"
    if mod.__name__ != 'vtkmodules.vtkCommonCore':
        base = 'vtkmodules.vtkCommonCore.' + base
    out = "class " + c.__name__ + "(" + base + "):\n"
    count = 0

    # do all nested classes (these will be enum types)
    items = topologically_sorted_items(c.__dict__)
    others = []
    for m,o in items:
        if isenum(o) and m == o.__name__:
            out += add_indent(class_pyi(o), "    ")
            count += 1
        else:
            others.append((m, o))

    # do all constants
    items = others
    others = []
    for m,o in items:
        if not m.startswith("__") and not ismethod(o) and not isclass(o):
            out += "    " + m + ":" + typename_forward(o) + "\n"
            count += 1
        else:
            others.append((m,o))

    if count == 0:
        out = out[0:-1] + " ...\n"

    return out

def class_pyi(c):
    """Generate all the method stubs for a class.
    """
    bases = []
    for b in c.__bases__:
        if b.__module__ in (c.__module__, 'builtins'):
            bases.append(b.__name__)
        else:
            bases.append(b.__module__ + "." + b.__name__)

    out = "class " + c.__name__ + "(" + ", ".join(bases) + "):\n"
    count = 0

    # do all nested classes (these are usually enum types)
    items = topologically_sorted_items(c.__dict__)
    others = []
    for m,o in items:
        if isclass(o) and m == o.__name__:
            out += add_indent(class_pyi(o), "    ")
            count += 1
        else:
            others.append((m, o))

    # do all constants
    items = others
    others = []
    for m,o in items:
        if not m.startswith("__") and not ismethod(o) and not isclass(o):
            out += "    " + m + ":" + typename_forward(o) + "\n"
            count += 1
        else:
            others.append((m,o))

    # do the __init__ methods
    constructors = get_constructors(c)
    if len(constructors) == 0:
        #if hasattr(c, "__init__") and not issubclass(c, int):
        #    out += "    def __init__() -> None: ...\n"
        #    count += 1
        pass
    else:
        count += 1
        if len(constructors) == 1:
            out += make_def(constructors[0], "    ") + "\n"
        else:
            for overload in constructors:
                out += make_def("@overload\n" + overload, "    ") + "\n"

    # do the methods
    items = others
    others = []
    for m,o in items:
        if ismethod(o):
            signatures = get_signatures(o)
            if len(signatures) == 0:
                continue
            count += 1
            if len(signatures) == 1:
                 out += make_def(signatures[0], "    ") + "\n"
                 continue
            for overload in signatures:
                 out += make_def("@overload\n" + overload, "    ") + "\n"
        else:
            others.append((m, o))

    if count == 0:
        out = out[0:-1] + " ...\n"

    return out

def module_pyi(mod, output):
    """Generate the contents of a .pyi file for a VTK module.
    """
    # needed stuff from typing module
    output.write("from typing import overload, Any, Callable, TypeVar, Union\n\n")
    output.write("Callback = Union[Callable[..., None], None]\n")
    output.write("Buffer = TypeVar('Buffer')\n")
    output.write("Pointer = TypeVar('Pointer')\n")
    output.write("Template = TypeVar('Template')\n")
    output.write("\n")

    if mod.__name__ == 'vtkmodules.vtkCommonCore':
        # dummy superclass for namespaces
        output.write("class namespace: pass\n")
        output.write("\n")

    # all the modules this module depends on
    depends = set(['vtkmodules.vtkCommonCore'])
    for m,o in mod.__dict__.items():
        if isclass(o) and m == o.__name__:
            for base in o.__bases__:
                depends.add(base.__module__)
    depends.discard(mod.__name__)
    depends.discard("builtins")
    for depend in sorted(depends):
        output.write("import " + depend + "\n")
    if depends:
        output.write("\n")

    # sort the dict according to dependency
    items = topologically_sorted_items(mod.__dict__)

    # do all namespaces
    others = []
    for m,o in items:
        if isnamespace(o) and m == o.__name__:
            output.write(namespace_pyi(o, mod))
            output.write("\n")
        else:
            others.append((m, o))

    # do all enum types
    items = others
    others = []
    for m,o in items:
        if isenum(o) and m == o.__name__:
            output.write(class_pyi(o))
            output.write("\n")
        else:
            others.append((m, o))

    # do all enum aliases
    items = others
    others = []
    for m,o in items:
        if isenum(o) and m != o.__name__:
            output.write(m + " = " + o.__name__ + "\n")
        else:
            others.append((m, o))

    # do all constants
    items = others
    others = []
    for m,o in items:
        if not m.startswith("__") and not ismethod(o) and not isclass(o):
            output.write(m + ":" + typename_forward(o) + "\n")
        else:
            others.append((m,o))
    if len(items) > len(others):
       output.write("\n")

    # do all classes
    items = others
    others = []
    for m,o in items:
        if isclass(o) and m == o.__name__:
            output.write(class_pyi(o))
            output.write("\n")
        else:
            others.append((m, o))

    # do all class aliases
    items = others
    others = []
    for m,o in items:
        if isclass(o) and m != o.__name__:
            output.write(m + " = " + o.__name__ + "\n")
        else:
            others.append((m, o))

def main(argv=sys.argv):
    # for error messages etcetera
    progname = os.path.basename(argv[0])

    # parse the program arguments
    parser = argparse.ArgumentParser(
        prog=argv[0],
        usage="python " + progname + " [-p package] [-o output_dir]",
        description="A .pyi generator for the VTK python wrappers.")
    parser.add_argument('-p', '--package', type=str, default="vtkmodules",
                        help="Package name [vtkmodules].")
    parser.add_argument('-o', '--output', type=str,
                        help="Output directory [package directory].")
    parser.add_argument('-e', '--ext', type=str, default=".pyi",
                        help="Output file suffix [.pyi].")
    parser.add_argument('--test', action='count', default=0,
                        help="Test .pyi files instead of creating them.")
    parser.add_argument('modules', type=str, nargs='*',
                        help="Modules to process [all].")
    args = parser.parse_args(argv[1:])

    # for convenience
    packagename = args.package
    modules = args.modules
    basedir = args.output
    ext = args.ext

    # get information about the package
    if basedir is None or len(modules) == 0:
        mod = importlib.import_module(packagename)
    if basedir is None:
        filename = getattr(mod, '__file__', None)
        if filename is None or os.path.basename(filename) != '__init__.py':
            sys.stderr.write(progname + ": " + packagename + " has no __init__.py\n")
            return 1
        basedir = os.path.dirname(filename)
    if len(modules) == 0:
        for modname in mod.__all__:
            # only generate .pyi files for the extension modules in __all__
            try:
                spec = importlib.util.find_spec(packagename + "." + modname)
            except ValueError:
                spec = None
                if not errflag:
                    errflag = True
                    sys.stderr.write(progname + ": couldn't get loader for " + modname + "\n")
            if spec is None:
                continue
            if not isinstance(spec.loader, importlib.machinery.ExtensionFileLoader):
                continue
            # the module is definitely an extension module
            modules.append(modname)

    # iterate through the modules in the package
    errflag = False
    for modname in modules:
        pyifile = os.path.join(basedir, modname + ext)
        if args.test:
            # test the syntax of the .pyi file
            flags = ast.PyCF_TYPE_COMMENTS if sys.hexversion >= 0x3080000 else 0
            with open(pyifile, 'r') as f:
                compile(f.read(), pyifile, 'exec', flags)
        else:
            # generate the .pyi file for the module
            mod = importlib.import_module(packagename + "." + modname)
            with open(pyifile, "w") as f:
                module_pyi(mod, f)

if __name__ == '__main__':
    result = main(sys.argv)
    if result is not None:
        sys.exit(result)
