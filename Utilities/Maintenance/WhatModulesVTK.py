#!/usr/bin/env python3

import re
from collections import defaultdict
from pathlib import Path


def get_program_parameters():
    import argparse
    description = 'Generate a find_package(VTK COMPONENTS ...) that lists all modules referenced by a set of files.'
    epilogue = '''
This uses the VTK source folder to determine the modules and headers.
Then the user files/folders are looked at to find the headers being used.
Finally a find_package() is output with the modules you need for
 inclusion in your CMakeLists.txt file.

Note:
1) If include file(s) are not found when building your application.
    You may need to search the VTK source to find where the file is.
    Then look at contents of vtk.module in that folder and add the
     module name to the find_package statement.
2) If linking fails, it usually means that the needed module has not been
     built, so you may need to add it to your VTK build and rebuild VTK.
3) More modules than strictly necessary may be included.
    '''
    parser = argparse.ArgumentParser(description=description, epilog=epilogue,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('vtk_path', help='The path to the VTK source tree.')
    parser.add_argument('application', nargs='+', help='Paths to the application files or folders.')
    args = parser.parse_args()
    return args.vtk_path, args.application


def check_paths(vtk_src_dir, application_srcs):
    """
    Check that the paths are valid.

    :param vtk_src_dir: The path to the VTK source.
    :param application_srcs: The user source files to be scanned for modules.
    :return: True if paths, files exist.
    """
    ok = True
    if not Path(vtk_src_dir).is_dir():
        print('The path to your VTK Source folder does not exist.')
        print(Path(vtk_src_dir))
        ok = False
    bad_paths = list()
    for f in application_srcs:
        p = Path(f)
        if not (p.is_dir() or p.is_file()):
            bad_paths.append(p)
    if bad_paths:
        print('These application paths or files do not exist.')
        for p in bad_paths:
            print(p)
            ok = False
    return ok


def find_vtk_modules(vtk_src_dir):
    """
    Build a dict of the VTK Module name, library name(if it exists) and any header files.

    :param vtk_src_dir: The path to the VTK source folder
    :return: Module name, library name and headers.
    """
    vtk_modules = defaultdict(dict)
    modules = [f for f in Path(vtk_src_dir).rglob('vtk.module')]
    # Includes in the module path
    file_patterns = ['vtk*.h', 'vtk*.h.in', 'QVTK*.h', '*QtQuick.h']
    for module in modules:
        content = module.read_text()
        args = content.split('\n')
        #  Assuming NAME is always there.
        if 'NAME' in args:
            name = args[args.index('NAME') + 1].strip()
            if 'LIBRARY_NAME' in args:
                library_name = args[args.index('LIBRARY_NAME') + 1].strip()
            else:
                library_name = None
            headers = list()
            for pattern in file_patterns:
                if pattern == 'vtk*.h.in':
                    # These files will be converted to header files in the build folder of VTK.
                    headers.extend([f.with_suffix('').name for f in module.parent.glob(pattern)])
                else:
                    headers.extend([f.name for f in module.parent.glob(pattern)])
            vtk_modules[name]['library_name'] = library_name
            vtk_modules[name]['headers'] = headers
    return vtk_modules


def build_headers_modules(modules):
    """
    Make a dictionary whose key is the header filename and value is the module.

    :param modules: The modules.
    :return: Headers and their corresponding module.
    """
    # The headers should be unique to a module, however we will not assume this.
    headers_modules = defaultdict(set)
    for k, v in modules.items():
        if 'headers' in v:
            for k1 in v['headers']:
                headers_modules[k1].add(k)
    return headers_modules


def find_application_includes(path):
    """
    Build a set that contains the vtk includes found in the file.

    :param path: The path to the application file.
    :return: The includes that were found.
    """
    includes = set()
    include_hdr1 = re.compile(r'((?:vtk|QVTK).*\.h)')
    include_hdr2 = re.compile(r'(\w+QtQuick\.h)')
    content = path.read_text()
    incs = include_hdr1.findall(content)
    includes.update(incs)
    incs = include_hdr2.findall(content)
    includes.update(incs)
    return includes


def generate_find_package(vtk_src_dir, application_srcs):
    """
    Generate the find_package statement.

    :param vtk_src_dir: The VTK source folder.
    :param application_srcs: A list of application folders and or files.
    :return: The find_package statement.
    """
    vtk_modules = find_vtk_modules(vtk_src_dir)
    # Test to see if VTK source is provided
    if len(vtk_modules) == 0:
        print(vtk_src_dir, 'is not a VTK source directory. It does not contain any vtk.module files.')
        return None
    vtk_headers_modules = build_headers_modules(vtk_modules)

    valid_extensions = ['.h', '.hxx', '.txx', '.cpp', '.cxx', '.cc']

    # Build a set of includes for all command line files
    all_includes = set()
    for app_src in application_srcs:
        p = Path(app_src)
        if p.is_file():
            if p.suffix in valid_extensions:
                all_includes.update(find_application_includes(p))
        elif p.is_dir():
            paths = list()
            for ext in valid_extensions:
                paths.extend([f for f in p.rglob('*' + ext) if f.is_file()])
            for path in paths:
                all_includes.update(find_application_includes(path))
    if len(all_includes) == 0:
        print('No VTK includes found in the application files.')
        return None

    # Build a set that contains all modules referenced in the user files.
    all_modules = set()
    for inc in all_includes:
        if inc in vtk_headers_modules:
            for m in vtk_headers_modules[inc]:
                all_modules.add(m)

    if 'VTK::RenderingCore' in all_modules:
        all_modules.add('VTK::RenderingOpenGL2')
        all_modules.add('VTK::InteractionStyle')
        all_modules.add('VTK::RenderingFreeType')
        all_modules.add('VTK::RenderingGL2PSOpenGL2')
        all_modules.add('VTK::RenderingContextOpenGL2')
    if 'VTK::DomainsChemistry' in all_modules:
        all_modules.add('VTK::DomainsChemistryOpenGL2')
    if 'VTK::RenderingVolume' in all_modules:
        all_modules.add('VTK::RenderingVolumeOpenGL2')
    if 'VTK::RenderingContext2D' in all_modules:
        all_modules.add('VTK::RenderingContextOpenGL2')
    if 'VTK::IOExport' in all_modules:
        all_modules.add('VTK::RenderingContextOpenGL2')
        all_modules.add('VTK::IOExportOpenGL2')
        all_modules.add('VTK::IOExportPDF')
        all_modules.add('VTK::RenderingContextOpenGL2')

    res = ['All modules referenced in your files:', 'find_package(VTK', ' COMPONENTS']
    for m in sorted(all_modules):
        res.append(' ' * 2 + m.replace('VTK::', ''))
    res.append(')')
    res.append(
        'Your application code includes ' + str(len(all_modules)) + ' of ' + str(
            len(vtk_modules)) + ' vtk modules.')
    return res


def main():
    vtk_src_dir, application_srcs = get_program_parameters()
    if not check_paths(vtk_src_dir, application_srcs):
        return

    res = generate_find_package(vtk_src_dir, application_srcs)
    if res:
        print('\n'.join(res))


if __name__ == '__main__':
    print('If you have built VTK and\n modules.json is available in the build folder.')
    print(' Please consider using FindNeededModules.py instead.')
    main()
