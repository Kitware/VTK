""" This module loads the entire VTK library into its namespace.  It
also allows one to use specific packages inside the vtk directory.."""

import __helper

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

# removing things the user shouldn't have to see.
del __helper
