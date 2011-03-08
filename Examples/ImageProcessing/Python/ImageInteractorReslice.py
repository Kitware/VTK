#!/usr/bin/env python

# A simple vtkInteractorStyleImage example for
# 3D image viewing with the vtkImageResliceMapper.

# Drag Left mouse button to window/level
# Shift-Left drag to rotate (oblique slice)
# Shift-Middle drag to slice through image
# OR Ctrl-Right drag to slice through image

import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

reader = vtk.vtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataMask(0x7fff)
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetDataSpacing(3.2,3.2,1.5)
reader.SetFilePrefix("" + str(VTK_DATA_ROOT) + "/Data/headsq/quarter")

# Create the RenderWindow, Renderer
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

im = vtk.vtkImageResliceMapper()
im.SetInputConnection(reader.GetOutputPort())
im.SliceFacesCameraOn()
im.SliceAtFocalPointOn()
im.BorderOff()

ip = vtk.vtkImageProperty()
ip.SetColorWindow(2000)
ip.SetColorLevel(1000)
ip.SetAmbient(0.0)
ip.SetDiffuse(1.0)
ip.SetOpacity(1.0)
ip.SetInterpolationTypeToLinear()

ia = vtk.vtkImageSlice()
ia.SetMapper(im)
ia.SetProperty(ip)

ren1.AddViewProp(ia)
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(300,300)

iren = vtk.vtkRenderWindowInteractor()
style = vtk.vtkInteractorStyleImage()
style.SetInteractionModeToImage3D()
iren.SetInteractorStyle(style)
renWin.SetInteractor(iren)

# render the image
renWin.Render()
cam1 = ren1.GetActiveCamera()
cam1.ParallelProjectionOn()
ren1.ResetCameraClippingRange()
renWin.Render()

iren.Start()
