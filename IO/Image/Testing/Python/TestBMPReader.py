#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkBMPReader
from vtkmodules.vtkImagingCore import vtkImageMapToColors
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# this test is designed to check the operation of the 8bit
# export of BMPs
# Image pipeline
reader = vtkBMPReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/masonry.bmp")
reader.SetAllow8BitBMP(1)
map = vtkImageMapToColors()
map.SetInputConnection(reader.GetOutputPort())
map.SetLookupTable(reader.GetLookupTable())
map.SetOutputFormatToRGB()
viewer = vtkImageViewer()
viewer.SetInputConnection(map.GetOutputPort())
viewer.SetColorWindow(256)
viewer.SetColorLevel(127.5)
#make interface
viewer.Render()
# --- end of script --
