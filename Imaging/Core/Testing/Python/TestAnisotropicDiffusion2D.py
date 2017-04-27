#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Image pipeline
reader = vtk.vtkPNGReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/fullhead15.png")
diffusion = vtk.vtkImageAnisotropicDiffusion2D()
diffusion.SetInputConnection(reader.GetOutputPort())
diffusion.SetDiffusionFactor(1.0)
diffusion.SetDiffusionThreshold(200.0)
diffusion.SetNumberOfIterations(5)
#diffusion DebugOn
viewer = vtk.vtkImageViewer()
#viewer DebugOn
viewer.SetInputConnection(diffusion.GetOutputPort())
viewer.SetColorWindow(3000)
viewer.SetColorLevel(1500)
viewer.Render()
# --- end of script --
