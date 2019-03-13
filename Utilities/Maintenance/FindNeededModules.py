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
 
You can choose any of these approaches along with the path to modules.json:
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
    parser.add_argument('-j', '--json', nargs='?', const=1, type=str, default="modules.json",
                        help='The path to the VTK JSON file (modules.json).')
    parser.add_argument('source', help='The path to the source files or to a file.')
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
                print('No such file:', path)
                exit()
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
            print('Unrecognised extension:', path)
            exit()
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
    From the parsed JASON data file make a dictionary whose key is the
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


def disp_components(modules):
    """
    For the found modules display them in a form that the user can
     copy/paste into their CMakeLists.txt file.
    :param modules: The modules
    :return:
    """
    res = 'find_package(VTK\n COMPONENTS\n'
    for m in sorted(modules):
        res += '    {:s}\n'.format(m.split('::')[1])
    res += ')'
    return res


def main():
    json_fn, src = get_program_parameters()
    # Assume everything we need is in modules.json.
    with open(json_fn) as data_file:
        json_data = json.load(data_file)
    headers_modules = build_headers_modules(json_data)

    if os.path.isdir(src):
        headers = get_users_headers(src)
        if not bool(headers):
            print('No files were found in the path:', src)
            exit()
    else:
        headers = get_users_file_headers(src)
        if not bool(headers):
            print('File or path does not exist:', src)
            exit()

    modules = set()
    inc_no_mod = set()
    for incl in headers:
        if incl in headers_modules:
            m = headers_modules[incl]
            for v in m:
                modules.add(v)
        else:
            inc_no_mod.add(incl)

    # These are added by default:
    for m in json_data['modules']:
        # If rendering is used we need OpenGL
        if 'RenderingOpenGL' in m:
            modules.add(m)
        # We usually need to interact with the image.
        if 'InteractionStyle' in m:
            modules.add(m)

    print(disp_components(modules))

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
            sh = sorted(list(headers[i]))
            nohead += '\n {:s} in:'.format(i)
            nohead += '\n   '
            nohead += '\n   '.join([', '.join(sh[j:j + 3]) for j in range(0, len(sh), 3)])
        nohead += '\n' + line + '\n'
        print(nohead)


if __name__ == '__main__':
    main()
