# These are always built with VTK. If they are not found, throw
# an exception
try:
    from vtkCommonPython import *
except ImportError:
    from libvtkCommonPython import *
try:
    from vtkFilteringPython import *
except ImportError:
    from libvtkFilteringPython import *
try:
    from vtkIOPython import *
except ImportError:
    from libvtkIOPython import *
try:
    from vtkImagingPython import *
except ImportError:
    from libvtkImagingPython import *
try:
    from vtkGraphicsPython import *
except ImportError:
    from libvtkGraphicsPython import *

# These are optional. If they are not found, continue.
try:
    from vtkRenderingPython import *
except ImportError:
    try:
        from libvtkRenderingPython import *
    except:
        pass
try:
    from vtkHybridPython import *
except ImportError:
    try:
        from libvtkHybridPython import *
    except:
        pass
try:
    from vtkParallelPython import *
except ImportError:
    try:
        from libvtkParallelPython import *
    except:
        pass
try:
    from vtkPatentedPython import *
except ImportError:
    try:
        from libvtkPatentedPython import *
    except:
        pass
    

# Check the command line arguments for data path (-D)
import os, sys

for i in range(len(sys.argv)):
    if sys.argv[i] == '-D' and i < len(sys.argv)-1:
        root = sys.argv[i+1]

# Check if the env. variable VTK_DATA_ROOT is set
try:
    VTK_DATA_ROOT = root
except NameError:
    try:
        VTK_DATA_ROOT = os.environ['VTK_DATA_ROOT']
    except KeyError:
        VTK_DATA_ROOT = './'

