#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkBMPReader
from vtkmodules.vtkInteractionImage import vtkImageViewer2
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

reader = vtkBMPReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/masonry.bmp")
# set the window/level
viewer = vtkImageViewer2()
viewer.SetInputConnection(reader.GetOutputPort())
viewer.SetColorWindow(100.0)
viewer.SetColorLevel(127.5)
viewer.Render()
# --- end of script --
