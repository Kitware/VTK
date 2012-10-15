#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# this demonstrates appending data to generate an implicit model
# contrast this with appendImplicitModel.tcl which set the bounds
# explicitly. this scrip should produce the same results.
cubeForBounds = vtk.vtkCubeSource()
cubeForBounds.SetBounds(-2.5,2.5,-2.5,2.5,-2.5,2.5)
cubeForBounds.Update()
lineX = vtk.vtkLineSource()
lineX.SetPoint1(-2.0,0.0,0.0)
lineX.SetPoint2(2.0,0.0,0.0)
lineX.Update()
lineY = vtk.vtkLineSource()
lineY.SetPoint1(0.0,-2.0,0.0)
lineY.SetPoint2(0.0,2.0,0.0)
lineY.Update()
lineZ = vtk.vtkLineSource()
lineZ.SetPoint1(0.0,0.0,-2.0)
lineZ.SetPoint2(0.0,0.0,2.0)
lineZ.Update()
aPlane = vtk.vtkPlaneSource()
aPlane.Update()
# set Data(3) "lineX"
# set Data(1) "lineY"
# set Data(2) "lineZ"
# set Data(0) "aPlane"
imp = vtk.vtkImplicitModeller()
imp.SetSampleDimensions(60,60,60)
imp.SetCapValue(1000)
imp.ComputeModelBounds(cubeForBounds.GetOutput())
# Okay now let's see if we can append
imp.StartAppend()
# for {set i 0} {$i < 4} {incr i} {
#     imp Append [$Data($i) GetOutput]
# }
imp.Append(aPlane.GetOutput())
imp.Append(lineZ.GetOutput())
imp.Append(lineY.GetOutput())
imp.Append(lineX.GetOutput())
imp.EndAppend()
cf = vtk.vtkContourFilter()
cf.SetInputConnection(imp.GetOutputPort())
cf.SetValue(0,0.1)
mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(cf.GetOutputPort())
actor = vtk.vtkActor()
actor.SetMapper(mapper)
outline = vtk.vtkOutlineFilter()
outline.SetInputConnection(imp.GetOutputPort())
outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)
plane = vtk.vtkImageDataGeometryFilter()
plane.SetInputConnection(imp.GetOutputPort())
plane.SetExtent(0,60,0,60,30,30)
planeMapper = vtk.vtkPolyDataMapper()
planeMapper.SetInputConnection(plane.GetOutputPort())
planeMapper.SetScalarRange(0.197813,0.710419)
planeActor = vtk.vtkActor()
planeActor.SetMapper(planeMapper)
# graphics stuff
ren1 = vtk.vtkRenderer()
ren1.AddActor(actor)
ren1.AddActor(planeActor)
ren1.AddActor(outlineActor)
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren1.SetBackground(0.1,0.2,0.4)
renWin.Render()
ren1.GetActiveCamera().Azimuth(30)
ren1.GetActiveCamera().Elevation(30)
ren1.ResetCameraClippingRange()
renWin.Render()
# --- end of script --
