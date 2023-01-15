#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkImageReader
from vtkmodules.vtkImagingGeneral import vtkImageAnisotropicDiffusion3D
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Diffuses to 26 neighbors if difference is below threshold.
# Image pipeline
reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetFilePrefix(VTK_DATA_ROOT + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
reader.SetDataSpacing(1,1,2)
diffusion = vtkImageAnisotropicDiffusion3D()
diffusion.SetInputConnection(reader.GetOutputPort())
diffusion.SetDiffusionFactor(1.0)
diffusion.SetDiffusionThreshold(100.0)
diffusion.SetNumberOfIterations(5)
diffusion.GetExecutive().SetReleaseDataFlag(0,0)
viewer = vtkImageViewer()
#viewer DebugOn
viewer.SetInputConnection(diffusion.GetOutputPort())
viewer.SetZSlice(22)
viewer.SetColorWindow(3000)
viewer.SetColorLevel(1500)
#make interface
viewer.Render()
# --- end of script --
