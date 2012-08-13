#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
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
rk4 = vtk.vtkRungeKutta4()
streamer = vtk.vtkStreamLine()
streamer.SetInputData(output)
streamer.SetSourceData(ps.GetOutput())
streamer.SetMaximumPropagationTime(100)
streamer.SetIntegrationStepLength(.2)
streamer.SetStepLength(.001)
streamer.SetNumberOfThreads(1)
streamer.SetIntegrationDirectionToForward()
streamer.VorticityOn()
streamer.SetIntegrator(rk4)
rf = vtk.vtkRibbonFilter()
rf.SetInputConnection(streamer.GetOutputPort())
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
# prevent the tk window from showing up then start the event loop
# for testing
threshold = 15
# --- end of script --
