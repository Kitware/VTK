""" This module loads the entire VTK library into its namespace.  It
also allows one to use specific packages inside the vtk directory.."""

import os
import sys

# AIX apparently does not have dl?
try:
    import dl
except ImportError:
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
    from rendering import *
    kits.append('rendering')
except ImportError, exc:
    __helper.refine_import_err('rendering', exc)

try:
    from hybrid import *
    kits.append('hybrid')
except ImportError, exc:
    __helper.refine_import_err('hybrid', exc)

try:
    from patented import *
    kits.append('patented')
except ImportError, exc:
    __helper.refine_import_err('patented', exc)

try:
    from parallel import *
    kits.append('parallel')
except ImportError, exc:
    __helper.refine_import_err('parallel', exc)

# import useful VTK related constants.
from util.vtkConstants import *

# reset the dlopen flags to the original state if possible.
if dl and (os.name == 'posix') and orig_dlopen_flags:
    sys.setdlopenflags(orig_dlopen_flags)

# removing things the user shouldn't have to see.
del __helper, orig_dlopen_flags
del sys, dl, os
