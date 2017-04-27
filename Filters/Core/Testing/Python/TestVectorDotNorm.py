#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# this is a tcl version of plate vibration
ren1 = vtk.vtkRenderer()
ren2 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# read a vtk file
#
plate = vtk.vtkPolyDataReader()
plate.SetFileName(VTK_DATA_ROOT + "/Data/plate.vtk")
plate.SetVectorsName("mode8")

warp = vtk.vtkWarpVector()
warp.SetInputConnection(plate.GetOutputPort())
warp.SetScaleFactor(0.5)

normals = vtk.vtkPolyDataNormals()
normals.SetInputConnection(warp.GetOutputPort())

color = vtk.vtkVectorDot()
color.SetInputConnection(normals.GetOutputPort())

lut = vtk.vtkLookupTable()
lut.SetNumberOfColors(256)
lut.Build()

i = 0
while i < 128:
    lut.SetTableValue(i, (128.0 - i) / 128.0, (128.0 - i) / 128.0,
      (128.0 - i) / 128.0, 1)
    i += 1

i = 128
while i < 256:
    lut.SetTableValue(i, (i - 128.0) / 128.0, (i - 128.0) / 128.0,
      (i - 128.0) / 128.0, 1)
    i += 1

plateMapper = vtk.vtkDataSetMapper()
plateMapper.SetInputConnection(color.GetOutputPort())
plateMapper.SetLookupTable(lut)
plateMapper.SetScalarRange(-1, 1)

plateActor = vtk.vtkActor()
plateActor.SetMapper(plateMapper)

color2 = vtk.vtkVectorNorm()
color2.SetInputConnection(plate.GetOutputPort())
color2.NormalizeOn()

plateMapper2 = vtk.vtkDataSetMapper()
plateMapper2.SetInputConnection(color2.GetOutputPort())
plateMapper2.SetLookupTable(lut)
plateMapper2.SetScalarRange(0, 1)

plateActor2 = vtk.vtkActor()
plateActor2.SetMapper(plateMapper2)

# Add the actors to the renderer, set the background and size
#
ren1.SetViewport(0, 0, .5, 1)
ren2.SetViewport(.5, 0, 1, 1)

ren1.AddActor(plateActor)
ren1.SetBackground(1, 1, 1)
ren2.AddActor(plateActor2)
ren2.SetBackground(1, 1, 1)

renWin.SetSize(500, 250)

camera = vtk.vtkCamera()
camera.SetPosition(1,1,1)
ren1.SetActiveCamera(camera)
ren2.SetActiveCamera(camera)

ren1.ResetCamera()
renWin.Render()

# render the image
#
iren.Initialize()
#iren.Start()
