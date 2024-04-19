#!/usr/bin/env python
from vtkmodules.vtkCommonExecutionModel import vtkStreamingDemandDrivenPipeline
from vtkmodules.vtkFiltersGeometry import vtkStructuredGridGeometryFilter
from vtkmodules.vtkIOParallel import vtkPlot3DMetaReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCamera,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot

VTK_DATA_ROOT = vtkGetDataRoot()

renWin = vtkRenderWindow()

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

r = vtkPlot3DMetaReader()
r.SetFileName("%s/Data/test.p3d" % VTK_DATA_ROOT)

r.UpdateInformation()
outInfo = r.GetOutputInformation(0)
l = outInfo.Length(vtkStreamingDemandDrivenPipeline.TIME_STEPS())
if l != 2:
    raise "Error: wrong number of time steps: %d. Should be 2" % l

outInfo.Set(vtkStreamingDemandDrivenPipeline.UPDATE_TIME_STEP(), 3.5)
r.Update()

outInfo.Set(vtkStreamingDemandDrivenPipeline.UPDATE_TIME_STEP(), 4.5)
r.Update()

output = r.GetOutput().GetBlock(0)

plane = vtkStructuredGridGeometryFilter()
plane.SetInputData(output)
plane.SetExtent(25,25,0,100,0,100)

mapper = vtkPolyDataMapper()
mapper.SetInputConnection(plane.GetOutputPort())
mapper.SetScalarRange(output.GetPointData().GetScalars().GetRange())

actor = vtkActor()
actor.SetMapper(mapper)

ren = vtkRenderer()

camera = vtkCamera()
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
