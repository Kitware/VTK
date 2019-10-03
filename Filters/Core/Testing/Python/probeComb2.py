#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Test alternative methods of probing data including
# using vtkFindCellStrategy and directly specifying
# a cell locator.

# Control test size
res = 50

# create planes
# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
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

# Probe with three separate planes. Use different probing approaches on each
# of the three planes (vtkDataSet::FindCell(), directly specifying a cell
# locator, and using a vtkFindCellStrategy. Then isocontour the planes.

# First plane
plane = vtk.vtkPlaneSource()
plane.SetResolution(res,res)
transP1 = vtk.vtkTransform()
transP1.Translate(3.7,0.0,28.37)
transP1.Scale(5,5,5)
transP1.RotateY(90)
tpd1 = vtk.vtkTransformPolyDataFilter()
tpd1.SetInputConnection(plane.GetOutputPort())
tpd1.SetTransform(transP1)
probe1 = vtk.vtkProbeFilter()
probe1.SetInputConnection(tpd1.GetOutputPort())
probe1.SetSourceData(output)
probe1.DebugOn()
contour1 = vtk.vtkContourFilter()
contour1.SetInputConnection(probe1.GetOutputPort())
contour1.GenerateValues(50,output.GetScalarRange())
mapProbe1 = vtk.vtkPolyDataMapper()
mapProbe1.SetInputConnection(contour1.GetOutputPort())
mapProbe1.SetScalarRange(output.GetScalarRange())
probe1Actor = vtk.vtkActor()
probe1Actor.SetMapper(mapProbe1)

outTpd1 = vtk.vtkOutlineFilter()
outTpd1.SetInputConnection(tpd1.GetOutputPort())
mapTpd1 = vtk.vtkPolyDataMapper()
mapTpd1.SetInputConnection(outTpd1.GetOutputPort())
tpd1Actor = vtk.vtkActor()
tpd1Actor.SetMapper(mapTpd1)
tpd1Actor.GetProperty().SetColor(0,0,0)

# Next plane
transP2 = vtk.vtkTransform()
transP2.Translate(9.2,0.0,31.20)
transP2.Scale(5,5,5)
transP2.RotateY(90)
tpd2 = vtk.vtkTransformPolyDataFilter()
tpd2.SetInputConnection(plane.GetOutputPort())
tpd2.SetTransform(transP2)
cellLoc = vtk.vtkStaticCellLocator()
probe2 = vtk.vtkProbeFilter()
probe2.SetInputConnection(tpd2.GetOutputPort())
probe2.SetSourceData(output)
probe2.SetCellLocatorPrototype(cellLoc)
probe2.DebugOn()
contour2 = vtk.vtkContourFilter()
contour2.SetInputConnection(probe2.GetOutputPort())
contour2.GenerateValues(50,output.GetScalarRange())
mapProbe2 = vtk.vtkPolyDataMapper()
mapProbe2.SetInputConnection(contour2.GetOutputPort())
mapProbe2.SetScalarRange(output.GetScalarRange())
probe2Actor = vtk.vtkActor()
probe2Actor.SetMapper(mapProbe2)

outTpd2 = vtk.vtkOutlineFilter()
outTpd2.SetInputConnection(tpd2.GetOutputPort())
mapTpd2 = vtk.vtkPolyDataMapper()
mapTpd2.SetInputConnection(outTpd2.GetOutputPort())
tpd2Actor = vtk.vtkActor()
tpd2Actor.SetMapper(mapTpd2)
tpd2Actor.GetProperty().SetColor(0,0,0)

# Third plane
transP3 = vtk.vtkTransform()
transP3.Translate(13.27,0.0,33.30)
transP3.Scale(5,5,5)
transP3.RotateY(90)
tpd3 = vtk.vtkTransformPolyDataFilter()
tpd3.SetInputConnection(plane.GetOutputPort())
tpd3.SetTransform(transP3)
strategy = vtk.vtkCellLocatorStrategy()
probe3 = vtk.vtkProbeFilter()
probe3.SetInputConnection(tpd3.GetOutputPort())
probe3.SetSourceData(output)
probe3.SetFindCellStrategy(strategy)
probe3.DebugOn()
contour3 = vtk.vtkContourFilter()
contour3.SetInputConnection(probe3.GetOutputPort())
contour3.GenerateValues(50,output.GetScalarRange())
mapProbe3 = vtk.vtkPolyDataMapper()
mapProbe3.SetInputConnection(contour3.GetOutputPort())
mapProbe3.SetScalarRange(output.GetScalarRange())
probe3Actor = vtk.vtkActor()
probe3Actor.SetMapper(mapProbe3)

outTpd3 = vtk.vtkOutlineFilter()
outTpd3.SetInputConnection(tpd3.GetOutputPort())
mapTpd3 = vtk.vtkPolyDataMapper()
mapTpd3.SetInputConnection(outTpd3.GetOutputPort())
tpd3Actor = vtk.vtkActor()
tpd3Actor.SetMapper(mapTpd3)
tpd3Actor.GetProperty().SetColor(0,0,0)

# Create an outline around the structured grid
outline = vtk.vtkStructuredGridOutlineFilter()
outline.SetInputData(output)
outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)

# Add the actors
ren1.AddActor(outlineActor)
ren1.AddActor(probe1Actor)
ren1.AddActor(probe2Actor)
ren1.AddActor(probe3Actor)
ren1.AddActor(tpd1Actor)
ren1.AddActor(tpd2Actor)
ren1.AddActor(tpd3Actor)
ren1.SetBackground(1,1,1)
renWin.SetSize(400,400)

# Setup a camera view
cam1 = ren1.GetActiveCamera()
cam1.SetClippingRange(3.95297,50)
cam1.SetFocalPoint(8.88908,0.595038,29.3342)
cam1.SetPosition(-12.3332,31.7479,41.2387)
cam1.SetViewUp(0.060772,-0.319905,0.945498)

iren.Initialize()
renWin.Render()
iren.Start()
# --- end of script --
