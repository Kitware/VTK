#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
ren2 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create pipeline
#
pl3d = vtk.vtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName("" + str(VTK_DATA_ROOT) + "/Data/combxyz.bin")
pl3d.SetQFileName("" + str(VTK_DATA_ROOT) + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
output = pl3d.GetOutput().GetBlock(0)
ps = vtk.vtkPlaneSource()
ps.SetXResolution(4)
ps.SetYResolution(4)
ps.SetOrigin(2,-2,26)
ps.SetPoint1(2,2,26)
ps.SetPoint2(2,-2,32)
psMapper = vtk.vtkPolyDataMapper()
psMapper.SetInputConnection(ps.GetOutputPort())
psActor = vtk.vtkActor()
psActor.SetMapper(psMapper)
psActor.GetProperty().SetRepresentationToWireframe()

# Use the vtkModifiedBSPTree
rk4 = vtk.vtkRungeKutta4()
bspLoc = vtk.vtkModifiedBSPTree()
ivp = vtk.vtkCellLocatorInterpolatedVelocityField()
ivp.SetCellLocatorPrototype(bspLoc)
streamer = vtk.vtkStreamTracer()
streamer.SetInputData(output)
streamer.SetSourceData(ps.GetOutput())
streamer.SetMaximumPropagation(100)
streamer.SetInitialIntegrationStep(.2)
streamer.SetIntegrationDirectionToForward()
streamer.SetComputeVorticity(1)
streamer.SetIntegrator(rk4)
streamer.SetInterpolatorPrototype(ivp)

rf = vtk.vtkRibbonFilter()
rf.SetInputConnection(streamer.GetOutputPort())
rf.SetInputArrayToProcess(1, 0, 0, vtk.vtkDataObject.FIELD_ASSOCIATION_POINTS, "Normals")
rf.SetWidth(0.1)
rf.SetWidthFactor(5)

streamMapper = vtk.vtkPolyDataMapper()
streamMapper.SetInputConnection(rf.GetOutputPort())
streamMapper.SetScalarRange(output.GetScalarRange())
streamline = vtk.vtkActor()
streamline.SetMapper(streamMapper)

outline = vtk.vtkStructuredGridOutlineFilter()
outline.SetInputData(output)
outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)

# Use a vtkStaticCellLocator
staticLoc = vtk.vtkStaticCellLocator()
ivp2 = vtk.vtkCellLocatorInterpolatedVelocityField()
ivp2.SetCellLocatorPrototype(staticLoc)
streamer2 = vtk.vtkStreamTracer()
streamer2.SetInputData(output)
streamer2.SetSourceData(ps.GetOutput())
streamer2.SetMaximumPropagation(100)
streamer2.SetInitialIntegrationStep(.2)
streamer2.SetIntegrationDirectionToForward()
streamer2.SetComputeVorticity(1)
streamer2.SetIntegrator(rk4)
streamer2.SetInterpolatorPrototype(ivp2)
streamer2.Update()

rf2 = vtk.vtkRibbonFilter()
rf2.SetInputConnection(streamer2.GetOutputPort())
rf2.SetInputArrayToProcess(1, 0, 0, vtk.vtkDataObject.FIELD_ASSOCIATION_POINTS, "Normals")
rf2.SetWidth(0.1)
rf2.SetWidthFactor(5)

streamMapper2 = vtk.vtkPolyDataMapper()
streamMapper2.SetInputConnection(rf2.GetOutputPort())
streamMapper2.SetScalarRange(output.GetScalarRange())
streamline2 = vtk.vtkActor()
streamline2.SetMapper(streamMapper2)

outline2 = vtk.vtkStructuredGridOutlineFilter()
outline2.SetInputData(output)
outlineMapper2 = vtk.vtkPolyDataMapper()
outlineMapper2.SetInputConnection(outline2.GetOutputPort())
outlineActor2 = vtk.vtkActor()
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
