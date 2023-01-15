#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import vtkDataObject
from vtkmodules.vtkCommonMath import vtkRungeKutta4
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkFiltersCore import vtkStructuredGridOutlineFilter
from vtkmodules.vtkFiltersFlowPaths import vtkStreamTracer
from vtkmodules.vtkFiltersModeling import vtkRibbonFilter
from vtkmodules.vtkFiltersSources import vtkPlaneSource
from vtkmodules.vtkIOParallel import vtkMultiBlockPLOT3DReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
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

# Control test size
res = 2

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create pipeline
#
pl3d = vtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(VTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
output = pl3d.GetOutput().GetBlock(0)

# Create a rake of streamlines
ps = vtkPlaneSource()
ps.SetXResolution(res)
ps.SetYResolution(res)
ps.SetOrigin(2,-2,26)
ps.SetPoint1(2,2,26)
ps.SetPoint2(2,-2,32)
ps.Update()

psMapper = vtkPolyDataMapper()
psMapper.SetInputConnection(ps.GetOutputPort())

psActor = vtkActor()
psActor.SetMapper(psMapper)
psActor.GetProperty().SetRepresentationToWireframe()

rk4 = vtkRungeKutta4()
streamer = vtkStreamTracer()
streamer.SetInputData(output)
streamer.SetSourceData(ps.GetOutput())
streamer.SetMaximumPropagation(100)
streamer.SetInitialIntegrationStep(.2)
streamer.SetIntegrationDirectionToForward()
streamer.SetComputeVorticity(1)
streamer.SetIntegrator(rk4)

numStreamers = ps.GetOutput().GetNumberOfPoints()
timer = vtkTimerLog()
timer.StartTimer()
streamer.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Time to generate: {0} streamers".format(numStreamers), ": {0}".format(time))

rf = vtkRibbonFilter()
rf.SetInputConnection(streamer.GetOutputPort())
rf.SetInputArrayToProcess(1, 0, 0, vtkDataObject.FIELD_ASSOCIATION_POINTS, "Normals")
rf.SetWidth(0.1)
rf.SetWidthFactor(5)

streamMapper = vtkPolyDataMapper()
streamMapper.SetInputConnection(rf.GetOutputPort())
streamMapper.SetScalarRange(output.GetScalarRange())
streamline = vtkActor()
streamline.SetMapper(streamMapper)

print("Output scalar range:",output.GetScalarRange())

outline = vtkStructuredGridOutlineFilter()
outline.SetInputData(output)

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(psActor)
ren1.AddActor(outlineActor)
ren1.AddActor(streamline)
ren1.SetBackground(1,1,1)
renWin.SetSize(300,300)
ren1.SetBackground(0.1,0.2,0.4)
cam1 = ren1.GetActiveCamera()
cam1.SetClippingRange(3.95297,50)
cam1.SetFocalPoint(9.71821,0.458166,29.3999)
cam1.SetPosition(2.7439,-37.3196,38.7167)
cam1.SetViewUp(-0.16123,0.264271,0.950876)
# render the image
#
renWin.Render()
threshold = 15
iren.Start()
# --- end of script --
