#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# this is a tcl version of plate vibration
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# read a vtk file
#
plate = vtk.vtkPolyDataReader()
plate.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/plate.vtk")
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
    lut.SetTableValue(i,expr.expr(globals(), locals(),["(","128.0","-i",")/","128.0"]),expr.expr(globals(), locals(),["(","128.0","-i",")/","128.0"]),expr.expr(globals(), locals(),["(","128.0","-i",")/","128.0"]),1)
    i = i + 1

i = 128
while i < 256:
    lut.SetTableValue(i,expr.expr(globals(), locals(),["(","i","-128.0",")/","128.0"]),expr.expr(globals(), locals(),["(","i","-128.0",")/","128.0"]),expr.expr(globals(), locals(),["(","i","-128.0",")/","128.0"]),1)
    i = i + 1

plateMapper = vtk.vtkDataSetMapper()
plateMapper.SetInputConnection(color.GetOutputPort())
plateMapper.SetLookupTable(lut)
plateMapper.SetScalarRange(-1,1)
plateActor = vtk.vtkActor()
plateActor.SetMapper(plateMapper)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(plateActor)
ren1.SetBackground(1,1,1)
renWin.SetSize(250,250)
ren1.GetActiveCamera().SetPosition(13.3991,14.0764,9.97787)
ren1.GetActiveCamera().SetFocalPoint(1.50437,0.481517,4.52992)
ren1.GetActiveCamera().SetViewAngle(30)
ren1.GetActiveCamera().SetViewUp(-0.120861,0.458556,-0.880408)
ren1.GetActiveCamera().SetClippingRange(12.5724,26.8374)
# render the image
#
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
