#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Test the button source
# The image to map on the button
r = vtk.vtkJPEGReader()
r.SetFileName(VTK_DATA_ROOT + "/Data/beach.jpg")
r.Update()

t = vtk.vtkTexture()
t.SetInputConnection(r.GetOutputPort())

dims = r.GetOutput().GetDimensions()
d1 = dims[0]
d2 = dims[1]

# The first elliptical button
bs = vtk.vtkEllipticalButtonSource()
bs.SetWidth(2)
bs.SetHeight(1)
bs.SetDepth(0.2)
bs.SetCircumferentialResolution(64)
bs.SetRadialRatio(1.1)
bs.SetShoulderResolution(8)
bs.SetTextureResolution(4)
bs.TwoSidedOn()

bMapper = vtk.vtkPolyDataMapper()
bMapper.SetInputConnection(bs.GetOutputPort())

b1 = vtk.vtkActor()
b1.SetMapper(bMapper)
b1.SetTexture(t)

# The second elliptical button
bs2 = vtk.vtkEllipticalButtonSource()
bs2.SetWidth(2)
bs2.SetHeight(1)
bs2.SetDepth(0.2)
bs2.SetCircumferentialResolution(64)
bs2.SetRadialRatio(1.1)
bs2.SetShoulderResolution(8)
bs2.SetTextureResolution(4)
bs2.TwoSidedOn()
bs2.SetCenter(2, 0, 0)
bs2.SetTextureStyleToFitImage()
bs2.SetTextureDimensions(d1, d2)

b2Mapper = vtk.vtkPolyDataMapper()
b2Mapper.SetInputConnection(bs2.GetOutputPort())

b2 = vtk.vtkActor()
b2.SetMapper(b2Mapper)
b2.SetTexture(t)

# The third rectangular button
bs3 = vtk.vtkRectangularButtonSource()
bs3.SetWidth(1.5)
bs3.SetHeight(0.75)
bs3.SetDepth(0.2)
bs3.TwoSidedOn()
bs3.SetCenter(0, 1, 0)
bs3.SetTextureDimensions(d1, d2)

b3Mapper = vtk.vtkPolyDataMapper()
b3Mapper.SetInputConnection(bs3.GetOutputPort())

b3 = vtk.vtkActor()
b3.SetMapper(b3Mapper)
b3.SetTexture(t)

# The fourth rectangular button
bs4 = vtk.vtkRectangularButtonSource()
bs4.SetWidth(1.5)
bs4.SetHeight(0.75)
bs4.SetDepth(0.2)
bs4.TwoSidedOn()
bs4.SetCenter(2, 1, 0)
bs4.SetTextureStyleToFitImage()
bs4.SetTextureDimensions(d1, d2)

b4Mapper = vtk.vtkPolyDataMapper()
b4Mapper.SetInputConnection(bs4.GetOutputPort())

b4 = vtk.vtkActor()
b4.SetMapper(b4Mapper)
b4.SetTexture(t)

# Create the RenderWindow, Renderer and Interactive Renderer
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(b1)
ren1.AddActor(b2)
ren1.AddActor(b3)
ren1.AddActor(b4)
ren1.SetBackground(0, 0, 0)

renWin.SetSize(250, 150)

renWin.Render()
ren1.GetActiveCamera().Zoom(1.5)
renWin.Render()

iren.Initialize()
#iren.Start()
