#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkImageReader2Factory
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Image pipeline
createReader = vtkImageReader2Factory()
reader = createReader.CreateImageReader2(VTK_DATA_ROOT + "/Data/beach.jpg")
reader.SetFileName(VTK_DATA_ROOT + "/Data/beach.jpg")
viewer = vtkImageViewer()
viewer.SetInputConnection(reader.GetOutputPort())
viewer.SetColorWindow(256)
viewer.SetColorLevel(127.5)
#make interface
viewer.Render()
reader.UnRegister(viewer) # not needed in python
# --- end of script --
