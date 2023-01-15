#!/usr/bin/env python
from vtkmodules.vtkCommonTransforms import vtkTransform
from vtkmodules.vtkIOImage import vtkBMPReader
from vtkmodules.vtkImagingCore import vtkImageReslice
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# we have to make sure it works with multiple scalar components
# Image pipeline
reader = vtkBMPReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/masonry.bmp")
reader.SetDataExtent(0,255,0,255,0,0)
reader.SetDataSpacing(1,1,1)
reader.SetDataOrigin(0,0,0)
reader.UpdateWholeExtent()
transform = vtkTransform()
transform.RotateZ(45)
transform.Scale(1.414,1.414,1.414)
reslice = vtkImageReslice()
reslice.SetInputConnection(reader.GetOutputPort())
reslice.SetResliceTransform(transform)
reslice.InterpolateOn()
reslice.SetInterpolationModeToCubic()
reslice.WrapOn()
reslice.AutoCropOutputOn()
viewer = vtkImageViewer()
viewer.SetInputConnection(reslice.GetOutputPort())
viewer.SetZSlice(0)
viewer.SetColorWindow(256.0)
viewer.SetColorLevel(127.5)
viewer.Render()
# --- end of script --
