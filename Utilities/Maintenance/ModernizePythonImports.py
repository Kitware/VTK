#!/usr/bin/env python
"""
This program will parse Python test scripts and replace "import vtk"
with importation of individual classes from the VTK extension modules.
For sub-packages, "import vtk.x.y" will become "import vtkmodules.x,y".
Any "from vtk.x.y import z" or "from vtk.x.y import z as s" will be
similarly updated.
"""

import sys
import os
import re
import ast
import argparse
import importlib

import vtkmodules
import vtkmodules.util.vtkVariant

# make a set that contains the expected contents of vtkmodules
contents = set(vtkmodules.__all__ +
               ['__all__', '__doc__', '__file__', '__name__',
                '__package__', '__path__', '__version__'])

# create a reverse module dict
member_to_module = {}
for module in vtkmodules.__all__:
    m = importlib.import_module(f"vtkmodules.{module}")
    for member in m.__dict__:
        if member not in member_to_module:
            if not member.startswith('_'):
                member_to_module[member] = module

# include some exceptions from vtkmodules.all
member_to_module['calldata_type'] = 'util.misc'
member_to_module['vtkImageScalarTypeNameMacro'] = 'util.vtkConstants'
for member in vtkmodules.util.vtkVariant.__dict__:
    if member.startswith('vtkVariant'):
        member_to_module[member] = 'util.vtkVariant'

# create a dict of standard implementations
implementations = {
  'vtkInteractionImage' : ['vtkInteractionStyle', 'vtkRenderingFreeType', 'vtkRenderingOpenGL2'],
  'vtkRenderingCore' : ['vtkInteractionStyle', 'vtkRenderingFreeType', 'vtkRenderingOpenGL2'],
  'vtkRenderingVolume' : ['vtkRenderingVolumeOpenGL2'],
  'vtkRenderingContext2D' : ['vtkRenderingContextOpenGL2'],
  'vtkViewsContext2D' : ['vtkRenderingContextOpenGL2'],
 }

def error(filename, lines, node, explanation):
    """Print error information.
    """
    linetext = lines[node.lineno - 1].strip()
    sys.stderr.write(f"File \"{filename}\", line {node.lineno}\n")
    sys.stderr.write(f"    {linetext}\n")
    sys.stderr.write(f"Error: {explanation}\n");
    return True

def sort_modules(modules):
    """Sort alphabetically, but prioritize modules that start with 'vtk'
    or contain 'Common' or 'Core'. This semi-alphabetical sort is more
    pleasing to the eye than a topological sort.
    """
    # to sort by components, add spaces before capital letters
    modules = [re.sub("([A-Z]+[^A-Z]*)", " \\1", x) for x in modules]
    # add yet another space before 'Common' and 'Core' to prioritize
    modules = [re.sub("(Common|Core)\\b", " \\1", x) for x in modules]
    # and prioritize modules that start with 'vtk'
    modules = [re.sub("^(vtk)\\b", " \\1", x) for x in modules]
    modules.sort()
    # remove the added spaces to get the original module names
    modules = [re.sub(" *", "", x) for x in modules]
    return modules

def update(filename):
    """Update a python script in-place.
    No modifications are done if an error occurred.
    """
    err = None
    import_lineno = None
    replacements = []
    modules = {}

    with open(filename, encoding='utf-8') as f:
        lines = f.readlines()

    try:
        tree = ast.parse("".join(lines), filename)
    except SyntaxError as e:
        linetext = lines[e.lineno - 1].strip()
        sys.stderr.write(f"File \"{e.filename}\", line {e.lineno}\n")
        sys.stderr.write(f"    {linetext}\n")
        sys.stderr.write(f"Error: invalid syntax while parsing file at position {e.offset}.\n")
        return

    for node in ast.walk(tree):
        if isinstance(node, ast.Import):
            n = len(node.names)
            for item in node.names:
                if item.name.startswith('vtk'):
                    if n != 1:
                        err = error(filename, lines, item, "No handler for multiple imports.")
                    elif item.name == 'vtk':
                        if item.asname is not None:
                            err = error(filename, lines, item, "No handler for 'import vtk as'.")
                        elif import_lineno is not None:
                            err = error(filename, lines, item, "Multiple 'import vtk' statements.")
                        else:
                            # save line for "import vtk"
                            import_lineno = item.lineno
                    elif item.name.startswith('vtk.'):
                        # replace "import vtk.mod" with "import vtkmodules.mod"
                        newtext = "import vtkmodules.{module}".format(module=item.name[4:])
                        if item.asname is not None:
                            newtext += " as {name}".format(name=item.asname)
                        replacements.append((node, newtext))
        elif isinstance(node, ast.ImportFrom):
            if node.module.startswith("vtk."):
                # the aliases have "name" and "asname"
                newtext = "from vtkmodules.{module} import {items}".format(
                   module=node.module[4:],
                   items=",".join([(f"{x.name}" if x.asname is None
                                    else f"{x.name} as {x.asname}")
                                    for x in node.names]))
                replacements.append((node, newtext))
        elif isinstance(node, ast.Attribute):
            if isinstance(node.value, ast.Name) and node.value.id == 'vtk':
                if node.attr in contents:
                    # replace "vtk.attr" with "vtkmodules.attr"
                    replacements.append((node, f"vtkmodules.{node.attr}"))
                else:
                    # replace "vtk.attr" with "attr"
                    replacements.append((node, node.attr))
                    try:
                        members = modules.setdefault(member_to_module[node.attr], set())
                        members.add(node.attr)
                    except KeyError:
                        err = error(filename, lines, node, f"Name '{node.attr}' is not in any built module.")

    if err is not None:
        return

    if import_lineno is None:
        sys.stderr.write(f"Skipping: no 'import vtk' in file {filename}\n")
        return

    # gather the text replacements line-by-line
    rep_by_line = {}
    for rep in replacements:
        lineno = rep[0].lineno
        items = rep_by_line.setdefault(lineno, [])
        items.append(rep)

    # perform the replacements
    for lineno in sorted(rep_by_line):
        reps = [(x[0].col_offset, x[0].end_col_offset, x[1]) for x in rep_by_line[lineno]]
        line = lines[lineno - 1].encode('utf-8')
        for rep in sorted(reps, reverse=True):
            newtext = rep[2].encode('utf-8')
            line = line[:rep[0]] + newtext + line[rep[1]:]
        lines[lineno - 1] = line.decode('utf-8')

    # get line ending for added import lines
    import_line = lines[import_lineno - 1]
    if import_line.endswith('\r\n'):
        endl = '\r\n'
    else:
        endl = '\n'

    # get line indent for added import lines
    indent = import_line[0:len(import_line) - len(import_line.lstrip())]

    # import needed modules
    newimports = []
    impl_modules = set()
    for module in sort_modules(modules):
        members = modules[module]
        if len(members) <= 1:
            items = ", ".join(members)
            newimports.append(indent + f"from vtkmodules.{module} import {items}" + endl)
        else:
            newimports.append(indent + f"from vtkmodules.{module} import (" + endl)
            for name in sorted(members):
                newimports.append(indent + f"    {name}," + endl)
            newimports.append(indent + ")" + endl)
        if module in implementations:
            impl_modules.update(implementations[module])

    for module in sorted(impl_modules):
        newimports.append(indent + f"import vtkmodules.{module}" + endl)

    # replace 'import vtk' with vtkmodules imports
    lines[import_lineno - 1:import_lineno] = newimports

    with open(filename, 'w', encoding='utf-8') as f:
        f.writelines(lines)

def main(argv):
    prog = os.path.basename(argv[0])
    text = 'Update Python files to import vtkmodules instead of vtk.'
    parser = argparse.ArgumentParser(prog=prog, description=text)
    parser.add_argument('files', nargs='+',
                        help='Python source files to update.')
    args = parser.parse_args(argv[1:])

    for filename in args.files:
        update(filename)

if __name__ == '__main__':
    main(sys.argv)
