import sys, os
import vtkmodules
from vtkmodules.vtkCommonCore import vtkVersion

def vtkLoadPythonTkWidgets(interp):
    """vtkLoadPythonTkWidgets(interp) -- load vtk-tk widget extensions

    This is a mess of mixed python and tcl code that searches for the
    shared object file that contains the python-vtk-tk widgets.  Both
    the python path and the tcl path are searched.
    """
    X = vtkVersion.GetVTKMajorVersion()
    Y = vtkVersion.GetVTKMinorVersion()
    modname = 'vtkRenderingTk'
    name = '%s-%d.%d' % (modname,X,Y)
    pkgname = modname.lower().capitalize()

    # find out if the module is already loaded
    loadedpkgs = interp.call('info', 'loaded')
    found = False
    try:
        # check for result returned as a string
        found = (loadedpkgs.find(pkgname) >= 0)
    except AttributeError:
        # check for result returned as nested tuples
        for pkgtuple in loadedpkgs:
            found |= (pkgname in pkgtuple)
    if found:
        return

    # create the platform-dependent file name
    prefix = ''
    if sys.platform == 'cygwin':
        prefix = 'cyg'
    elif os.name == 'posix':
        prefix = 'lib'
    extension = interp.call('info', 'sharedlibextension')
    filename = prefix+name+extension

    # create an list of paths to search
    vtkmodules_dir = os.path.dirname(vtkmodules.__file__)
    pathlist = [vtkmodules_dir]

    # a likely relative path for linux
    if sys.platform == 'linux':
        package_dir = os.path.dirname(vtkmodules_dir)
        python_dir = os.path.dirname(package_dir)
        if os.path.basename(python_dir).startswith('python'):
            lib_dir = os.path.dirname(python_dir)
            if os.path.basename(lib_dir).startswith('lib'):
                pathlist.append(lib_dir)

    # add tcl paths, ensure that {} is handled properly
    try:
        auto_paths = interp.getvar('auto_path').split()
    except AttributeError:
        auto_paths = interp.getvar('auto_path')
    for path in auto_paths:
        prev = str(pathlist[-1])
        try:
            # try block needed when one uses Gordon McMillan's Python
            # Installer.
            if len(prev) > 0 and prev[0] == '{' and prev[-1] != '}':
                pathlist[-1] = prev+' '+path
            else:
                pathlist.append(path)
        except AttributeError:
            pass

    # a common installation path
    if os.name == 'posix':
        pathlist.append('/usr/local/lib')

    # attempt to load
    for path in pathlist:
        try:
            # If the path object is not str, it means that it is a
            # Tkinter path object.
            if not isinstance(path, str):
                path = path.string
            # try block needed when one uses Gordon McMillan's Python
            # Installer.
            if len(path) > 0 and path[0] == '{' and path[-1] == '}':
                path = path[1:-1]
            fullpath = os.path.join(path, filename)
        except AttributeError:
            pass
        if ' ' in fullpath:
            fullpath = '{'+fullpath+'}'
        if interp.eval('catch {load '+fullpath+' '+pkgname+'}') == '0':
            return

    # re-generate the error
    interp.call('load', filename, pkgname)
