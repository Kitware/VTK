#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# we have to make sure it works with multiple scalar components
# Image pipeline
reader = vtk.vtkBMPReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/masonry.bmp")
reader.SetDataExtent(0,255,0,255,0,0)
reader.SetDataSpacing(1,1,1)
reader.SetDataOrigin(0,0,0)
reader.UpdateWholeExtent()
transform = vtk.vtkTransform()
transform.RotateZ(45)
transform.Scale(1.414,1.414,1.414)
reslice = vtk.vtkImageReslice()
reslice.SetInputConnection(reader.GetOutputPort())
reslice.SetResliceTransform(transform)
reslice.InterpolateOn()
reslice.SetInterpolationModeToCubic()
reslice.WrapOn()
reslice.AutoCropOutputOn()
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(reslice.GetOutputPort())
viewer.SetZSlice(0)
viewer.SetColorWindow(256.0)
viewer.SetColorLevel(127.5)
viewer.Render()
# --- end of script --
