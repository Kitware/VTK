import sys, os, string

def vtkLoadPythonTkWidgets(interp):
    """vtkLoadPythonTkWidgets(interp) -- load vtk-tk widget extensions

    This is a mess of mixed python and tcl code that searches for the
    shared object file that contains the python-vtk-tk widgets.  Both
    the python path and the tcl path are searched.
    """
    name = 'vtkRenderingPythonTkWidgets'

    # find out if the file is already loaded
    loaded = interp.call('info', 'loaded')
    tkname = string.capitalize(string.lower(name))
    if string.find(loaded, tkname) >= 0:
        return

    # create the platform-dependent file name
    prefix = ''
    if os.name == 'posix':
        prefix = 'lib'
    extension = interp.call('info', 'sharedlibextension')
    filename = prefix+name+extension

    # create an extensive list of paths to search
    pathlist = sys.path + string.split(interp.getvar('auto_path'))
    if os.name == 'posix':
        pathlist.append('/usr/local/lib')

    # attempt to load
    for path in pathlist:
        fullpath = os.path.join(path, filename)
        if interp.eval('catch {load '+fullpath+'} errormsg') == '0':
            return

    # this is how to get the error from tcl (we don't use it, though)
    errormsg = interp.getvar('errormsg')
    
    # re-generate the error
    interp.call('load', filename)
    
