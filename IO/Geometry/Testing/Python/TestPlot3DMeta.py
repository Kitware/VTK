#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot

VTK_DATA_ROOT = vtkGetDataRoot()

renWin = vtk.vtkRenderWindow()

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

r = vtk.vtkPlot3DMetaReader()
r.SetFileName("%s/Data/test.p3d" % VTK_DATA_ROOT)

r.UpdateInformation()
outInfo = r.GetOutputInformation(0)
l = outInfo.Length(vtk.vtkStreamingDemandDrivenPipeline.TIME_STEPS())
if l != 2:
    raise "Error: wrong number of time steps: %d. Should be 2" % l

outInfo.Set(vtk.vtkStreamingDemandDrivenPipeline.UPDATE_TIME_STEP(), 3.5)
r.Update()

outInfo.Set(vtk.vtkStreamingDemandDrivenPipeline.UPDATE_TIME_STEP(), 4.5)
r.Update()

output = r.GetOutput().GetBlock(0)

plane = vtk.vtkStructuredGridGeometryFilter()
plane.SetInputData(output)
plane.SetExtent(25,25,0,100,0,100)

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(plane.GetOutputPort())
mapper.SetScalarRange(output.GetPointData().GetScalars().GetRange())

actor = vtk.vtkActor()
actor.SetMapper(mapper)

ren = vtk.vtkRenderer()

camera = vtk.vtkCamera()
ren.SetActiveCamera(camera)

renWin.AddRenderer(ren)
ren.AddActor(actor)

camera.SetViewUp(0,1,0)
camera.SetFocalPoint(0,0,0)
camera.SetPosition(1,0,0)
ren.ResetCamera()
camera.Dolly(1.25)
ren.ResetCameraClippingRange()

renWin.SetSize(400,300)
renWin.Render()

iren.Initialize()
