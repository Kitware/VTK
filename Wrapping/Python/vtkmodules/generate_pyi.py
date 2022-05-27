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
identifier = re.compile(r"""([A-Za-z_]([A-Za-z0-9_]|[.][A-Za-z_])*)""")
indent = re.compile(r"[ \t]+(?=\S)")
has_self = re.compile(r"[(]self[,)]")

# important characters for rapidly parsing code
keychar = re.compile(r"[\'\"{}\[\]()\n]")

def parse_error(message, text, begin, pos):
    """Print a parse error, syntax or otherwise.
    """
    end = text.find('\n', pos)
    if end == -1:
        end = len(text)
    sys.stderr.write("Error: " + message + ":\n")
    sys.stderr.write(text[begin:end] + "\n");
    sys.stderr.write('-' * (pos - begin) + "^\n")

def annotation_text(a, text, is_return):
    """Return the new text to be used for an annotation.
    """
    if isinstance(a, ast.Name):
        name = a.id
        if name not in types:
            # quote the type, in case it isn't yet defined
            text = '\'' + name + '\''
    elif isinstance(a, (ast.Tuple, ast.List)):
        size = len(a.elts)
        e = a.elts[0]
        offset = a.col_offset
        old_name = text[e.col_offset - offset:e.end_col_offset - offset]
        name = annotation_text(e, old_name, is_return)

        if is_return:
            # use concrete types for return values
            if isinstance(a, ast.Tuple):
                text = 'Tuple[' + ', '.join([name]*size) + ']'
            else:
                text = 'List[' + name + ']'
        else:
            # use generic sequence types for args
            if isinstance(a, ast.Tuple):
                text = 'Sequence[' + name + ']'
            else:
                text = 'MutableSequence[' + name + ']'

    return text

def fix_annotations(signature):
    """Fix the annotations in a method definition.
    The signature must be a single-line function def, no decorators.
    """
    # get the FunctionDef object from the parse tree
    definition = ast.parse(signature).body[0]
    annotations = [arg.annotation for arg in definition.args.args]
    return_i = len(annotations) # index of annotation for return
    annotations.append(definition.returns)

    # create a list of changes to apply to the annotations
    changes = []
    for i,a in enumerate(annotations):
        if a is not None:
            old_text = signature[a.col_offset:a.end_col_offset]
            text = annotation_text(a, old_text, (i == return_i))
            if text != old_text:
                changes.append((a.col_offset, a.end_col_offset,  text))

    # apply changes to generate a new signature
    if changes:
        newsig = ""
        lastpos = 0
        for begin,end,text in changes:
            newsig += signature[lastpos:begin]
            newsig += text
            lastpos = end
        newsig += signature[lastpos:]
        signature = newsig

    return signature

def push_signature(o, l, signature):
    """Process a method signature and add it to the list.
    """
    # eliminate newlines and indents
    signature = re.sub(r"\s+", " ", signature)
    # no space after opening delimiter or ':' or '='
    signature = re.sub(r"([({\[:=]) ", "\\1", signature)

    if signature.startswith('C++:'):
        # the C++ method signatures are unused
        pass
    elif signature.startswith(o.__name__ + "("):
        # make it into a python method definition
        signature = "def " + signature + ': ...'
        if sys.hexversion >= 0x3080000:
            # XXX(Python 3.8) uses ast features from 3.8
            signature = fix_annotations(signature)
        if signature not in l:
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
    # (this will go line-by-line or until first ( ) { } [ ] " ')
    while pos < len(doc):
        # look for the next "character of interest" in docstring
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
            elif c == '\n' and not (delim_stack or indent.match(doc, end)):
                # a newline not followed by an indent marks end of signature,
                # except for within brackets
                signature = doc[begin:pos].strip()
                if signature:
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
                if signature:
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
        if signature.startswith("def " + name + "("):
            signature = re.sub("-> \'?" + name + "\'?", "-> None", signature)
            if signature.startswith("def " + name + "()"):
                constructors.append(re.sub(name + r"\(", "__init__(self", signature, 1))
            else:
                constructors.append(re.sub(name + r"\(", "__init__(self, ", signature, 1))
    return constructors

def handle_static(o, signature):
    """If method has no "self", add @static decorator."""
    if isvtkmethod(o) and not has_self.search(signature):
        return "@staticmethod\n" + signature
    else:
        return signature

def add_indent(s, indent):
    """Add the given indent before every line in the string.
    """
    return indent + re.sub(r"\n(?=([^\n]))", "\n" + indent, s)

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
            out += add_indent(constructors[0], "    ") + "\n"
        else:
            for overload in constructors:
                out += add_indent("@overload\n" + overload, "    ") + "\n"

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
                 signature = handle_static(o, signatures[0])
                 out += add_indent(signature, "    ") + "\n"
                 continue
            for overload in signatures:
                 signature = handle_static(o, overload)
                 out += add_indent("@overload\n" + signature, "    ") + "\n"
        else:
            others.append((m, o))

    if count == 0:
        out = out[0:-1] + " ...\n"

    return out

def module_pyi(mod, output):
    """Generate the contents of a .pyi file for a VTK module.
    """
    # needed stuff from typing module
    output.write("from typing import overload, Any, Callable, TypeVar, Union\n")
    output.write("from typing import Tuple, List, Sequence, MutableSequence\n")
    output.write("\n")
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
