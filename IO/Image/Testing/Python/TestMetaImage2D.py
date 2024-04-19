#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkMetaImageReader
from vtkmodules.vtkInteractionImage import vtkImageViewer2
from vtkmodules.vtkRenderingCore import vtkRenderWindowInteractor
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This scripts shows a compressed spectrum of an image.
# Image pipeline
reader = vtkMetaImageReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/foot/foot.mha")

viewer = vtkImageViewer2()
viewer.SetInputConnection(reader.GetOutputPort())
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)

viewInt = vtkRenderWindowInteractor()
viewer.SetupInteractor(viewInt)
viewer.Render()

# This is needed if you want to interact with the image.
# viewInt.Start()
