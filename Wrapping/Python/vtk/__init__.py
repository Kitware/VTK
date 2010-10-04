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
    # do not give up too early:
    # are we on AMD64 ?
    try:
      import DLFCN as dl
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
from vtkCommonPython import *
from vtkFilteringPython import *
from vtkIOPython import *
from vtkImagingPython import *
from vtkGraphicsPython import *

# the vtk.kits variable tells us which kits we actually have
kits = ['common', 'filtering', 'io', 'imaging', 'graphics']

# Try to load optional kits.  The helper function checks if the
# ImportError is actually a link error.

try:
    from vtkGenericFilteringPython import *
    kits.append('genericfiltering')
except ImportError, exc:
    __helper.refine_import_err('genericfiltering', 'vtkGenericFilteringPython',
                               exc)
try:
    from vtkRenderingPython import *
    kits.append('rendering')
except ImportError, exc:
    __helper.refine_import_err('rendering', 'vtkRenderingPython', exc)

try:
    from vtkVolumeRenderingPython import *
    kits.append('volumerendering')
except ImportError, exc:
    __helper.refine_import_err('volumerendering',
                               'vtkVolumeRenderingPython', exc)

try:
    from vtkHybridPython import *
    kits.append('hybrid')
except ImportError, exc:
    __helper.refine_import_err('hybrid', 'vtkHybridPython', exc)

try:
    from vtkWidgetsPython import *
    kits.append('widgets')
except ImportError, exc:
    __helper.refine_import_err('widgets', 'vtkWidgetsPython', exc)

try:
    from vtkChartsPython import *
    kits.append('charts')
except ImportError, exc:
    __helper.refine_import_err('charts', 'vtkChartsPython', exc)

try:
    from vtkGeovisPython import *
    kits.append('geovis')
except ImportError, exc:
    __helper.refine_import_err('geovis', 'vtkGeovisPython', exc)

try:
    from vtkInfovisPython import *
    kits.append('infovis')
except ImportError, exc:
    __helper.refine_import_err('infovis', 'vtkInfovisPython', exc)

try:
    from vtkTextAnalysisPython import *
    kits.append('textanalysis')
except ImportError, exc:
    __helper.refine_import_err('textanalysis', 'vtkTextAnalysisPython', exc)

try:
    from vtkViewsPython import *
    kits.append('views')
except ImportError, exc:
    __helper.refine_import_err('views', 'vtkViewsPython', exc)

try:
    from vtkParallelPython import *
    kits.append('parallel')
except ImportError, exc:
    __helper.refine_import_err('parallel', 'vtkParallelPython', exc)

try:
    from qvtk import *
    kits.append('qvtk')
except ImportError, exc:
    __helper.refine_import_err('qvtk', 'vtkQtPython', exc)

# useful macro for getting type names
__vtkTypeNameDict = {VTK_VOID:"void",
                     VTK_DOUBLE:"double",
                     VTK_FLOAT:"float",
                     VTK_LONG:"long",
                     VTK_UNSIGNED_LONG:"unsigned long",
                     VTK_INT:"int",
                     VTK_UNSIGNED_INT:"unsigned int",
                     VTK_SHORT:"short",
                     VTK_UNSIGNED_SHORT:"unsigned short",
                     VTK_CHAR:"char",
                     VTK_UNSIGNED_CHAR:"unsigned char",
                     VTK_SIGNED_CHAR:"signed char",
                     VTK_LONG_LONG:"long long",
                     VTK_UNSIGNED_LONG_LONG:"unsigned long long",
                     VTK___INT64:"__int64",
                     VTK_UNSIGNED___INT64:"unsigned __int64",
                     VTK_ID_TYPE:"vtkIdType",
                     VTK_BIT:"bit"}

def vtkImageScalarTypeNameMacro(type):
  return __vtkTypeNameDict[type]

# import the vtkVariant helpers
from util.vtkVariant import *

# reset the dlopen flags to the original state if possible.
if dl and (os.name == 'posix') and orig_dlopen_flags:
    sys.setdlopenflags(orig_dlopen_flags)

# removing things the user shouldn't have to see.
del __helper, orig_dlopen_flags
del sys, dl, os
