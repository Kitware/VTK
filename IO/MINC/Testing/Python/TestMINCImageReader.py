#!/usr/bin/env python
from vtkmodules.vtkIOMINC import vtkMINCImageReader
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Image pipeline
reader = vtkMINCImageReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/t3_grid_0.mnc")

viewer = vtkImageViewer()
viewer.SetInputConnection(reader.GetOutputPort())
viewer.SetColorWindow(65535)
viewer.SetColorLevel(0)

# make interface
viewer.Render()
