#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkPNGReader
from vtkmodules.vtkImagingGeneral import vtkImageAnisotropicDiffusion2D
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Image pipeline
reader = vtkPNGReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/fullhead15.png")
diffusion = vtkImageAnisotropicDiffusion2D()
diffusion.SetInputConnection(reader.GetOutputPort())
diffusion.SetDiffusionFactor(1.0)
diffusion.SetDiffusionThreshold(200.0)
diffusion.SetNumberOfIterations(5)
#diffusion DebugOn
viewer = vtkImageViewer()
#viewer DebugOn
viewer.SetInputConnection(diffusion.GetOutputPort())
viewer.SetColorWindow(3000)
viewer.SetColorLevel(1500)
viewer.Render()
# --- end of script --
