#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
# this test has some wireframe geometry
# Make sure multisampling is disabled to avoid generating multiple
# regression images
# renWin SetMultiSamples 0
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# read data
#
reader = vtk.vtkStructuredGridReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/office.binary.vtk")
reader.Update()
#force a read to occur
outline = vtk.vtkStructuredGridOutlineFilter()
outline.SetInputConnection(reader.GetOutputPort())
mapOutline = vtk.vtkPolyDataMapper()
mapOutline.SetInputConnection(outline.GetOutputPort())
outlineActor = vtk.vtkActor()
outlineActor.SetMapper(mapOutline)
outlineActor.GetProperty().SetColor(0,0,0)
rk = vtk.vtkRungeKutta45()
# Create source for streamtubes
streamer = vtk.vtkStreamTracer()
streamer.SetInputConnection(reader.GetOutputPort())
streamer.SetStartPosition(0.1,2.1,0.5)
streamer.SetMaximumPropagation(500)
streamer.SetIntegrationStepUnit(2)
streamer.SetMinimumIntegrationStep(0.1)
streamer.SetMaximumIntegrationStep(1.0)
streamer.SetInitialIntegrationStep(0.2)
streamer.SetIntegrationDirection(0)
streamer.SetIntegrator(rk)
streamer.SetRotationScale(0.5)
streamer.SetMaximumError(1.0e-8)
aa = vtk.vtkAssignAttribute()
aa.SetInputConnection(streamer.GetOutputPort())
aa.Assign("Normals","NORMALS","POINT_DATA")
rf1 = vtk.vtkRibbonFilter()
rf1.SetInputConnection(aa.GetOutputPort())
rf1.SetWidth(0.1)
rf1.VaryWidthOff()
mapStream = vtk.vtkPolyDataMapper()
mapStream.SetInputConnection(rf1.GetOutputPort())
mapStream.SetScalarRange(reader.GetOutput().GetScalarRange())
streamActor = vtk.vtkActor()
streamActor.SetMapper(mapStream)
ren1.AddActor(outlineActor)
ren1.AddActor(streamActor)
ren1.SetBackground(0.4,0.4,0.5)
cam = ren1.GetActiveCamera()
cam.SetPosition(-2.35599,-3.35001,4.59236)
cam.SetFocalPoint(2.255,2.255,1.28413)
cam.SetViewUp(0.311311,0.279912,0.908149)
cam.SetClippingRange(1.12294,16.6226)
renWin.SetSize(300,200)
iren.Initialize()
# interact with data
# --- end of script --
