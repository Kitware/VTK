"""vtkpython -- the primary module for the vtk-python wrappers.

This module loads the various shared object files that make up
the vtk-python interface. 
"""

#-----------------------------------------------------------------------------
# load all of the kit modules into python 

import os

required_kits = ['Common','Filtering','IO','Imaging','Graphics']
optional_kits = ['Rendering','Hybrid','Parallel','Patented']

# the vtkpython.kits variable tells us which kits we actually have
kits = []

def vtkCreateKitModuleName(kit):
    """vtkCreateKitModuleName(kit) -- create module name from kit name
    """
    if os.name == 'posix':
        return 'libvtk'+kit+'Python'
    else:
        return 'vtk+'+kit+'Python'

# load the required kits
for kit in required_kits:
    kit_name = vtkCreateKitModuleName(kit)
    exec "from "+kit_name+" import *"
    kits.append(kit)

# try to load the optional kits
for kit in optional_kits:
    kit_name = vtkCreateKitModuleName(kit)
    try:
        exec "from "+kit_name+" import *"
        kits.append(kit)
    except ImportError:
        pass

#-----------------------------------------------------------------------------
# the following pieces are for the vtk regression testing and examples

import sys, os.path
from vtkGetDataRoot import vtkGetDataRoot
    
def vtkRegressionTestImage( renWin ):
    """vtkRegressionTestImage(renWin) -- produce regression image for window

    This function writes out a regression .png file for a vtkWindow.
    Does anyone involved in testing care to elaborate?
    """
    dl = vtkDebugLeaks()
    dl.PromptUserOff()
    imageIndex=-1;
    for i in range(0, len(sys.argv)):
        if sys.argv[i] == '-V' and i < len(sys.argv)-1:
            imageIndex = i+1

    if imageIndex != -1:
        fname = os.path.join(vtkGetDataRoot(), sys.argv[imageIndex])

        rt_w2if = vtkWindowToImageFilter()
        rt_w2if.SetInput(renWin)

        if os.path.isfile(fname):
            pass
        else:
            rt_pngw = vtkPNGWriter()
            rt_pngw.SetFileName(fname)
            rt_pngw.SetInput(rt_w2if.GetOutput())
            rt_pngw.Write()
            rt_pngw = None

        rt_png = vtkPNGReader()
        rt_png.SetFileName(fname)

        rt_id = vtkImageDifference()
        rt_id.SetInput(rt_w2if.GetOutput())
        rt_id.SetImage(rt_png.GetOutput())
        rt_id.Update()

        if rt_id.GetThresholdedError() <= 10:
            return 1
        else:
            sys.stderr.write('Failed image test: %f\n'
                             % rt_id.GetThresholdedError())
            return 0
    return 2
            
