# coding=utf8
"""
Usage:
    python3 ./Utilities/Marshalling/write_marshal_macro.py

This script opts in vtk classes for automatic marshalling.

Headers that already use these marshal macros are untouched.

Tip: Run clang-format on all modified files before commit.
"""

import argparse
import json
import os
import pathlib
import re
import sys

if sys.version_info < (3, 6):
    print("This script requires Python 3.6 or higher")
    sys.exit(1)

DATA_FILE = "marshal_modules.json"
MARSHAL_HINT_REGEX = r"VTK_MARSHAL(AUTO|MANUAL)"
MARSHAL_HINT_REGEXES = [r"VTK_MARSHALAUTO", r"VTK_MARSHALMANUAL"]
MODULE_HEADER_REGEX = r'^#include "vtk.*Module\.h"'
MODULE_EXPORT_REGEX = r"^class VTK.*_EXPORT"
WRAPHINT_HEADER_REGEX = r'^#include "vtkWrappingHints\.h"'

SCRIPT_DIR = pathlib.Path(os.path.dirname(__file__))
VTK_DIR = SCRIPT_DIR.parent.parent

MARSHALAUTO_TXT = SCRIPT_DIR.joinpath("VTK_MARSHALAUTO.txt")
MARSHALMANUAL_TXT = SCRIPT_DIR.joinpath("VTK_MARSHALMANUAL.txt")
IGNORE_TXT = SCRIPT_DIR.joinpath("ignore.txt")

headers = {IGNORE_TXT: set(), MARSHALAUTO_TXT: set(), MARSHALMANUAL_TXT: set()}
module_dirs = set()

"""
Return the line number and re.Match of the text matches the regex.
"""


def find_matching_line(filename, regex_str):
    result = []
    with open(filename, 'r', encoding='utf8') as f:
        for line_num, line in enumerate(f, start=1):
            match = re.search(regex_str, line)
            if match is not None:
                return line_num, match

    return None


"""
Return true if line is made up of 3 or more words separated by forward slash '/'
character. False otherwise.
"""


def is_valid_line(line, line_num, path):
    if len(line.split("/")) != 3:
        print(
            f"ERROR: No. of components must be greater than 3 in {path}:{line_num}")
        print(f"ERROR: Got {line}. Expected 'Module/Name/vtkClassName.h'")
        return False
    elif not line.endswith(".h"):
        print(f"ERROR: File is not a recognized header in {path}:{line_num}")
        print(f"ERROR: Got {line}. Expected 'Module/Name/vtkClassName.h'")
        return False
    else:
        return True


"""
Return true if a header in a module is present in atleast one of
VTK_MARSHALAUTO.txt (or) VTK_MARSHALMANUAL.txt (or) ignore.txt
with the correct annotation, or lack of the macro when the header is in ignore.txt
"""


def get_status():
    success = True

    # fail if any of the headers in `VTK_MARSHAL(AUTO|MANUAL).txt` don't have marshal macro
    for marshal_file, macro_regex in zip([MARSHALAUTO_TXT, MARSHALMANUAL_TXT], MARSHAL_HINT_REGEXES):
        with open(marshal_file, 'r', encoding='utf8') as f:
            lines = f.read().splitlines()
            for line_num, line in enumerate(lines, start=1):
                if not is_valid_line(line, line_num, marshal_file):
                    return False
                else:
                    parts = line.split('/')
                    module_dir = VTK_DIR.joinpath(*parts[0:-1])
                    header = module_dir.joinpath(parts[-1])
                    headers[marshal_file].add(header)
                    existing_macro_line_match = find_matching_line(
                        header, macro_regex)
                    if existing_macro_line_match is None:
                        print(
                            f"ERROR: {header} is missing {marshal_file.stem} macro")
                        success &= False

    # fail if the header is in `ignore.txt` and uses a marshal macro (excluding `vtkWrappingHints.h`)
    # fail if the header is not in any of `VTK_MARSHAL(AUTO|MANUAL).txt` and `ignore.txt`
    for module_dir in module_dirs:
        files = os.listdir(module_dir)
        for filename in files:
            if filename.endswith(".h"):
                header = module_dir.joinpath(filename)
                if header in headers[IGNORE_TXT]:
                    if header in headers[MARSHALAUTO_TXT]:
                        print(
                            f"ERROR: {header} cannot be in {MARSHALAUTO_TXT} and {IGNORE_TXT}")
                        success &= False
                    if header in headers[MARSHALMANUAL_TXT]:
                        print(
                            f"ERROR: {header} cannot be in {MARSHALMANUAL_TXT} and {IGNORE_TXT}")
                        success &= False
                    existing_macro_line_match = find_matching_line(
                        header, MARSHAL_HINT_REGEX)
                    module_export_line_match = find_matching_line(
                        header, MODULE_EXPORT_REGEX)
                    if existing_macro_line_match is not None and module_export_line_match is not None:
                        print(
                            f"ERROR: {header} (from ignore.txt) cannot have {existing_macro_line_match[1].group(0)} macro.")
                        success &= False
                elif header not in headers[MARSHALAUTO_TXT]:
                    if header not in headers[MARSHALMANUAL_TXT]:
                        print(
                            f"ERROR: {header} is missing from {IGNORE_TXT}")
                        success &= False

    return success


"""
Adds VTK_MARSHAL(AUTO|MANUAL) macro to header files listed in `VTK_MARSHAL(AUTO|MANUAL).txt`
Also adds a line that includes `vtkWrappingHints.h` if not already included.
"""


def update():
    for marshal_file, macro_regex in zip([MARSHALAUTO_TXT, MARSHALMANUAL_TXT], MARSHAL_HINT_REGEXES):
        with open(marshal_file, 'r', encoding='utf8') as f:
            lines = f.read().splitlines()
            for line_num, line in enumerate(lines, start=1):
                if not is_valid_line(line, line_num, marshal_file):
                    return False
                parts = line.split('/')
                module_dir = VTK_DIR.joinpath(*parts[0:-1])
                header = module_dir.joinpath(parts[-1])
                module_dirs.add(module_dir)
                headers[marshal_file].add(header)

                # Skip files that already have the marshal hint macro
                existing_macro_line_match = find_matching_line(
                    header, macro_regex)
                module_export_line_match = find_matching_line(
                    header, MODULE_EXPORT_REGEX)
                has_exported_class = module_export_line_match is not None
                if has_exported_class and existing_macro_line_match is not None:
                    continue
                else:
                    module_header_line_match = find_matching_line(
                        header, MODULE_HEADER_REGEX)
                    if has_exported_class and module_header_line_match is not None:
                        # Replaces the target strings in memory
                        new_lines = []
                        with open(header, 'r', encoding='utf8') as f:
                            new_lines = f.readlines()
                            # Replaces "class VTK*_EXPORT ..." with "class VTK*_EXPORT VTK_MARSHAL(AUTO|MANUAL) ..."
                            export_line_num, export_text_match = module_export_line_match
                            export_line_text = new_lines[export_line_num - 1]
                            # Remove existing macro
                            export_line_text = export_line_text.replace(
                                MARSHAL_HINT_REGEXES[0] + " ", "").replace(MARSHAL_HINT_REGEXES[1] + " ", "")
                            target = export_text_match.group()
                            replacement = f"{target} {marshal_file.stem}"
                            new_lines[export_line_num - 1] = export_line_text.replace(
                                target, replacement)
                            # Includes vtkWrappingHints.h below the include of the vtk*Module.h
                            wrap_hint_header_line_match = find_matching_line(
                                header, WRAPHINT_HEADER_REGEX)
                            # Remove existing line
                            if wrap_hint_header_line_match is not None:
                                dst_line = wrap_hint_header_line_match[0] - 1
                                new_lines[dst_line] = f'#include "vtkWrappingHints.h" // For {marshal_file.stem}\n'
                            else:
                                new_lines.insert(
                                    module_header_line_match[0], f'#include "vtkWrappingHints.h" // For {marshal_file.stem}\n')
                        # Writes the final result to the file
                        if len(new_lines):
                            with open(header, 'w', encoding='utf8') as f:
                                print(
                                    f'Write {marshal_file.stem} in {header}')
                                f.writelines(new_lines)

    # remove marshal macro for headers in `ignore.txt`
    for ignored_header in headers[IGNORE_TXT]:
        # Replaces the target strings in memory
        new_lines = []
        with open(ignored_header, 'r', encoding='utf8') as f:
            existing_macro_line_match = find_matching_line(
                ignored_header, MARSHAL_HINT_REGEX)
            wrap_hint_header_line_match = find_matching_line(
                ignored_header, WRAPHINT_HEADER_REGEX)
            module_export_line_match = find_matching_line(
                ignored_header, MODULE_EXPORT_REGEX)
            has_exported_class = module_export_line_match is not None
            # Replaces "class VTK*_EXPORT VTK_MARSHAL(AUTO|MANUAL) ..." with "class VTK*_EXPORT ..."
            if has_exported_class and existing_macro_line_match is not None:
                new_lines = f.readlines()
                export_line_num, export_text_match = module_export_line_match
                export_line_text = new_lines[export_line_num - 1]
                target = existing_macro_line_match[1].group()
                new_lines[export_line_num - 1] = export_line_text.replace(
                    target, '')
            # Removes vtkWrappingHints.h below the include of the vtk*Module.h
            if has_exported_class and wrap_hint_header_line_match is not None:
                if not len(new_lines):
                    new_lines = f.read_lines()
                wrappings_hints_line = new_lines[wrap_hint_header_line_match[0] - 1]
                new_lines.remove(wrappings_hints_line)
        # Writes the final result to the file
        if len(new_lines):
            with open(ignored_header, 'w', encoding='utf8') as f:
                print(
                    f'Remove marshal macro in {ignored_header}')
                f.writelines(new_lines)

    # Adds all headers not found in `VTK_MARSHAL(AUTO|MANUAL).txt`
    # into `ignore.txt`
    ignore_files = []
    for module_dir in module_dirs:
        filenames = os.listdir(module_dir)
        for filename in filenames:
            filepath = module_dir.joinpath(filename)
            if not filename.endswith(".h"):
                continue
            if filepath not in headers[MARSHALAUTO_TXT] and filepath not in headers[MARSHALMANUAL_TXT]:
                line = '/'.join(filepath.relative_to(VTK_DIR).parts)
                ignore_files.append(line)

    with open(IGNORE_TXT, 'w', encoding='utf8') as f:
        ignore_files.sort()
        ignore_files.append('')
        f.write('\n'.join(ignore_files))

    return True


if __name__ == "__main__":

    parser = argparse.ArgumentParser(
        prog='marshal_macro_annotate_headers',
        description='Analyze and annotate headers for marshalling')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("-t", "--test", action='store_true')
    group.add_argument("-u", "--update", action='store_true')
    args = parser.parse_args()

    # read ignored files
    with open(IGNORE_TXT, 'r', encoding='utf8') as f:
        lines = f.read().splitlines()
        for line_num, line in enumerate(lines, start=1):
            if not is_valid_line(line, line_num, IGNORE_TXT):
                exit(1)
            else:
                parts = line.split('/')
                module_dir = VTK_DIR.joinpath(*parts[0:-1])
                header = module_dir.joinpath(parts[-1])
                headers[IGNORE_TXT].add(header)

    if args.test:
        if not get_status():
            exit(1)
    elif args.update:
        if not update():
            exit(1)

    print('OK')
