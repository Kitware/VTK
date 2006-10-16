""" This module loads the entire VTK library into its namespace.  It
also allows one to use specific packages inside the vtk directory.."""

import os
import sys


# The dl module is used to force the symbols in the loaded VTK modules to
# be global, that is, to force symbols to be shared between modules.  This
# used to be necessary in VTK 4 but might not be with VTK 5 and later.

# The first "except" is because systems like AIX don't have the dl module.
# The second "except" is because the dl module raises a system error on
# ia64 and x86_64 systems because "int" and addresses are different sizes.
try:
    import dl
except ImportError:
    dl = None
except SystemError:
    dl = None

import __helper

# set the dlopen flags so that VTK does not run into problems with
# shared symbols.
try:
    # only Python >= 2.2 has this functionality
    orig_dlopen_flags = sys.getdlopenflags()
except AttributeError:
    orig_dlopen_flags = None

if dl and (os.name == 'posix'):
    sys.setdlopenflags(dl.RTLD_NOW|dl.RTLD_GLOBAL)    

# Load all required kits.
from common import *
from filtering import *
from io import *
from imaging import *
from graphics import *

# the vtk.kits variable tells us which kits we actually have
kits = ['common', 'filtering', 'io', 'imaging', 'graphics']

# Try to load optional kits.  The helper function checks if the
# ImportError is actually a link error.

try:
    from genericfiltering import *
    kits.append('genericfiltering')
except ImportError, exc:
    __helper.refine_import_err('genericfiltering', 'vtkGenericFilteringPython',
                               exc)
try:
    from rendering import *
    kits.append('rendering')
except ImportError, exc:
    __helper.refine_import_err('rendering', 'vtkRenderingPython', exc)

try:
    from volumerendering import *
    kits.append('volumerendering')
except ImportError, exc:
    __helper.refine_import_err('volumerendering',
                               'vtkVolumeRenderingPython', exc)

try:
    from hybrid import *
    kits.append('hybrid')
except ImportError, exc:
    __helper.refine_import_err('hybrid', 'vtkHybridPython', exc)

try:
    from widgets import *
    kits.append('widgets')
except ImportError, exc:
    __helper.refine_import_err('widgets', 'vtkWidgetsPython', exc)

try:
    from infovis import *
    kits.append('infovis')
except ImportError, exc:
    __helper.refine_import_err('infovis', 'vtkInfovisPython', exc)

try:
    from parallel import *
    kits.append('parallel')
except ImportError, exc:
    __helper.refine_import_err('parallel', 'vtkParallelPython', exc)

# import useful VTK related constants.
from util.vtkConstants import *

# reset the dlopen flags to the original state if possible.
if dl and (os.name == 'posix') and orig_dlopen_flags:
    sys.setdlopenflags(orig_dlopen_flags)

# removing things the user shouldn't have to see.
del __helper, orig_dlopen_flags
del sys, dl, os
