#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import (
    vtkCellLocatorStrategy,
    vtkCellTreeLocator,
    vtkDataObject,
    vtkStaticCellLocator,
)
from vtkmodules.vtkCommonMath import vtkRungeKutta4
from vtkmodules.vtkFiltersCore import vtkStructuredGridOutlineFilter
from vtkmodules.vtkFiltersFlowPaths import (
    vtkCompositeInterpolatedVelocityField,
    vtkStreamTracer,
)
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

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
ren2 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
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
ps = vtkPlaneSource()
ps.SetXResolution(4)
ps.SetYResolution(4)
ps.SetOrigin(2,-2,26)
ps.SetPoint1(2,2,26)
ps.SetPoint2(2,-2,32)
psMapper = vtkPolyDataMapper()
psMapper.SetInputConnection(ps.GetOutputPort())
psActor = vtkActor()
psActor.SetMapper(psMapper)
psActor.GetProperty().SetRepresentationToWireframe()

# Use the vtkCellTreeLocator
rk4 = vtkRungeKutta4()
treeLoc = vtkCellTreeLocator()
ivp = vtkCompositeInterpolatedVelocityField()
ivp.SetFindCellStrategy(vtkCellLocatorStrategy())
ivp.GetFindCellStrategy().SetCellLocator(treeLoc)
streamer = vtkStreamTracer()
streamer.SetInputData(output)
streamer.SetSourceData(ps.GetOutput())
streamer.SetMaximumPropagation(100)
streamer.SetInitialIntegrationStep(.2)
streamer.SetIntegrationDirectionToForward()
streamer.SetComputeVorticity(1)
streamer.SetIntegrator(rk4)
streamer.SetInterpolatorPrototype(ivp)

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

outline = vtkStructuredGridOutlineFilter()
outline.SetInputData(output)
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)

# Use a vtkStaticCellLocator
staticLoc = vtkStaticCellLocator()
ivp2 = vtkCompositeInterpolatedVelocityField()
ivp2.SetFindCellStrategy(vtkCellLocatorStrategy())
ivp2.GetFindCellStrategy().SetCellLocator(staticLoc)
streamer2 = vtkStreamTracer()
streamer2.SetInputData(output)
streamer2.SetSourceData(ps.GetOutput())
streamer2.SetMaximumPropagation(100)
streamer2.SetInitialIntegrationStep(.2)
streamer2.SetIntegrationDirectionToForward()
streamer2.SetComputeVorticity(1)
streamer2.SetIntegrator(rk4)
streamer2.SetInterpolatorPrototype(ivp2)
streamer2.Update()

rf2 = vtkRibbonFilter()
rf2.SetInputConnection(streamer2.GetOutputPort())
rf2.SetInputArrayToProcess(1, 0, 0, vtkDataObject.FIELD_ASSOCIATION_POINTS, "Normals")
rf2.SetWidth(0.1)
rf2.SetWidthFactor(5)

streamMapper2 = vtkPolyDataMapper()
streamMapper2.SetInputConnection(rf2.GetOutputPort())
streamMapper2.SetScalarRange(output.GetScalarRange())
streamline2 = vtkActor()
streamline2.SetMapper(streamMapper2)

outline2 = vtkStructuredGridOutlineFilter()
outline2.SetInputData(output)
outlineMapper2 = vtkPolyDataMapper()
outlineMapper2.SetInputConnection(outline2.GetOutputPort())
outlineActor2 = vtkActor()
outlineActor2.SetMapper(outlineMapper2)

# Add the actors to the renderer, set the background and size
#
ren1.SetViewport(0,0,0.5,1)
ren1.AddActor(psActor)
ren1.AddActor(outlineActor)
ren1.AddActor(streamline)
ren1.SetBackground(0.1,0.2,0.4)

ren2.SetViewport(0.5,0,1,1)
ren2.AddActor(psActor)
ren2.AddActor(outlineActor2)
ren2.AddActor(streamline2)
ren2.SetBackground(0.1,0.2,0.4)

renWin.SetSize(600,300)

cam1 = ren1.GetActiveCamera()
cam1.SetClippingRange(3.95297,50)
cam1.SetFocalPoint(9.71821,0.458166,29.3999)
cam1.SetPosition(2.7439,-37.3196,38.7167)
cam1.SetViewUp(-0.16123,0.264271,0.950876)

ren2.SetActiveCamera(cam1)

# render the image
#
renWin.Render()
# prevent the tk window from showing up then start the event loop
# for testing
threshold = 15
iren.Start()

# --- end of script --
