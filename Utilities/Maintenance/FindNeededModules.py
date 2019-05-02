#!/usr/bin/env python

from __future__ import print_function

import collections
import json
import os
import re


def get_program_parameters():
    import argparse
    description = 'Generate a find_package(VTK COMPONENTS ...) command for CMake.'
    epilogue = '''
Use modules.json and your source files to generate a
  find_package(VTK COMPONENTS ...) command listing all the vtk modules
  needed by the C++ source and header files in your code.
  
Paths to more than one modules.json file and/or more than one source path can be specified.
Note than if there are spaces in the paths, enclose the path in quotes.
 
You can choose any of these approaches along with the path(s) to modules.json:
1) Specify a path to your source files and headers.
   In this case this path all subdirectories are also searched
   for files with the usual usual C++ extensions.
2) Specify a specific file.
3) Specify a filename with no extensions.
   In this case the usual C++ extensions will be added automatically
   and the corresponding files (if they exist) searched.
 
If it is unable to find modules for your headers then
  a list of these, along with the files they are in, is produced
  so you can manually add the corresponding modules or rebuild VTK
  to include the missing modules.

You will need to manually add any third-party modules
   (if used) to the find_package command.
 
    '''
    parser = argparse.ArgumentParser(description=description, epilog=epilogue,
                                     formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('-j', '--json', nargs='+', default=["modules.json"],
                        help='The path to the VTK JSON file (modules.json).')
    parser.add_argument('-s', '--source', nargs='+', help='The path to the source files or to a file.')
    args = parser.parse_args()
    return args.json, args.source


class Constants:
    """
    Needed constants.
    """

    def __init__(self):
        self.header_pattern = re.compile(r'^#include *[<\"](\S+)[>\"]')
        self.vtk_include_pattern = re.compile(r'^(vtk\S+)')
        self.valid_ext = ['.h', '.hxx', '.cxx', '.cpp', '.txx']


def get_users_headers(path):
    """
    Get the set of VTK headers in all the user's files in the path.
    :param path: The starting path, all sub-paths will be searched.
    :return: headers as the key and the corresponding filenames as the value.
    """
    c = Constants()
    headers = collections.defaultdict(set)
    for root, dirs, files in os.walk(path):
        for f in files:
            name, ext = os.path.splitext(f)
            if ext:
                if ext in c.valid_ext:
                    with open(os.path.join(root, f)) as data:
                        # Read the file looking for includes.
                        for line in data:
                            m = c.header_pattern.match(line.strip())
                            if m:
                                # We have a header name, split it from its path (if the path exists).
                                header_parts = os.path.split(m.group(1))
                                m = c.vtk_include_pattern.match(header_parts[1])
                                if m:
                                    headers[m.group(1)].add(f)
    return headers


def get_users_file_headers(path):
    """
    Get the set of VTK headers in a user's file or files with a common name in the path.
    :param path: The file name or the file name without the extension.
    :return: headers as the key and the corresponding filenames as the value.
    """
    c = Constants()
    headers = collections.defaultdict(set)
    name, ext = os.path.splitext(path)
    if ext:
        if ext in c.valid_ext:
            if not os.path.isfile(path):
                raise Exception('No such file: ' + path)
            with open(path) as data:
                # Read the file looking for includes.
                for line in data:
                    m = c.header_pattern.match(line.strip())
                    if m:
                        # We have a header name, split it from its path (if the path exists).
                        header_parts = os.path.split(m.group(1))
                        m = c.vtk_include_pattern.match(header_parts[1])
                        if m:
                            headers[m.group(1)].add(os.path.split(path)[1])
        else:
            raise Exception('Unrecognised extension:' + path)
    else:
        for ext in c.valid_ext:
            fn = name + ext
            if os.path.isfile(fn):
                with open(fn) as data:
                    # Read the file looking for includes.
                    for line in data:
                        m = c.header_pattern.match(line.strip())
                        if m:
                            # We have a header name, split it from its path (if the path exists).
                            header_parts = os.path.split(m.group(1))
                            m = c.vtk_include_pattern.match(header_parts[1])
                            if m:
                                headers[m.group(1)].add(os.path.split(path)[1])
    return headers


def build_headers_modules(json_data):
    """
    From the parsed JSON data file make a dictionary whose key is the
     header filename and value is the module.
    :param json_data: The parsed JSON file modules.json.
    :return:
    """
    # The headers should be unique to a module, however we will not assume this.
    res = collections.defaultdict(set)
    for k, v in json_data['modules'].items():
        if 'headers' in v:
            for k1 in v['headers']:
                res[k1].add(k)
    return res


def disp_components(modules, module_implements):
    """
    For the found modules display them in a form that the user can
     copy/paste into their CMakeLists.txt file.
    :param modules: The modules.
    :param module_implements: Modules implementing other modules.
    :return:
    """
    res = 'find_package(VTK\n COMPONENTS\n'
    for m in sorted(modules):
        res += '    {:s}\n'.format(m.split('::')[1])
    if module_implements:
        keys = sorted(module_implements)
        max_width = len(max(keys, key=len).split('::')[1])
        res += '    # These modules are suggested since they implement an existing module.\n'
        res += '    # Uncomment those you need.\n'
        for key in keys:
            res += '    # {:<{width}} # implements {:s}\n'.format(key.split('::')[1], ', '.join(sorted(module_implements[key])),
                                                                  width=max_width)
    res += ')'
    return res


def main():
    json_paths, src_paths = get_program_parameters()
    # json_paths always has a default value.
    # However we require at least one value for src_paths.
    assert src_paths is not None, 'Enter a source file or folder name.'

    modules = set()
    inc_no_mod = set()
    inc_no_mod_headers = collections.defaultdict(set)
    mod_implements = collections.defaultdict(set)
    for fn in src_paths:
        if os.path.isdir(fn):
            headers = get_users_headers(fn)
            if not bool(headers):
                print('No files were found in the path:', fn)
        else:
            headers = get_users_file_headers(fn)
            if not bool(headers):
                print('File or path does not exist:', fn)

        for jfn in json_paths:
            # Assume everything we need is in modules.json.
            with open(jfn) as data_file:
                json_data = json.load(data_file)
            headers_modules = build_headers_modules(json_data)

            for incl in headers:
                if incl in headers_modules:
                    m = headers_modules[incl]
                    for v in m:
                        modules.add(v)
                else:
                    inc_no_mod.add(incl)
                    inc_no_mod_headers[incl] = headers[incl]

            if headers:
                for m in modules:
                    if not json_data['modules'][m]['implementable']:
                        continue
                    for i in json_data['modules']:
                        if i in modules:
                            continue
                        if m in json_data['modules'][i]['implements']:
                            # Suggest module i since it implements m
                            mod_implements[i].add(m)

    print(disp_components(modules, mod_implements))

    if inc_no_mod:
        line = '*' * 64
        nohead = '\n' + line + '\n'
        nohead += 'You will need to manually add the modules that\n'
        nohead += '  use these headers to the find_package command.\n'
        nohead += 'These could be external modules not in the modules.json file.\n'
        nohead += 'Or you may need to rebuild VTK to include the missing modules.\n'
        nohead += '\nHere are the vtk headers and corresponding files:'
        sinmd = sorted(inc_no_mod)
        for i in sinmd:
            sh = sorted(list(inc_no_mod_headers[i]))
            nohead += '\n {:s} in:'.format(i)
            nohead += '\n   '
            nohead += '\n   '.join([', '.join(sh[j:j + 3]) for j in range(0, len(sh), 3)])
        nohead += '\n' + line + '\n'
        print(nohead)


if __name__ == '__main__':
    main()
