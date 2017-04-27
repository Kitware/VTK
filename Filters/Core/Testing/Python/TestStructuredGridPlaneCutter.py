#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Control debugging parameters
res = 50

# Create the RenderWindow, Renderer
#
ren0 = vtk.vtkRenderer()
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create a synthetic source: sample a sphere across a volume
sphere = vtk.vtkSphere()
sphere.SetCenter(0.0,0.0,0.0)
sphere.SetRadius(0.25)

sample = vtk.vtkSampleFunction()
sample.SetImplicitFunction(sphere)
sample.SetModelBounds(-0.5,0.5, -0.5,0.5, -0.5,0.5)
sample.SetSampleDimensions(res,res,res)
sample.Update()

# Handy dandy filter converts image data to structured grid
convert = vtk.vtkImageDataToPointSet()
convert.SetInputConnection(sample.GetOutputPort())
convert.Update()
input = convert.GetOutput()

# Create a cutting plane
plane = vtk.vtkPlane()
plane.SetOrigin(input.GetCenter())
plane.SetNormal(1,1,1)

# First create the usual cutter
cutter = vtk.vtkCutter()
cutter.SetInputData(input)
cutter.SetCutFunction(plane)

cutterMapper = vtk.vtkPolyDataMapper()
cutterMapper.SetInputConnection(cutter.GetOutputPort())
cutterMapper.ScalarVisibilityOff()

cutterActor = vtk.vtkActor()
cutterActor.SetMapper(cutterMapper)
cutterActor.GetProperty().SetColor(1,1,1)

# Throw in an outline
outline = vtk.vtkOutlineFilter()
outline.SetInputData(input)

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)

# Now create the accelerated version.
sCutter = vtk.vtkPlaneCutter()
sCutter.SetInputData(input)
sCutter.SetPlane(plane)

sCutterMapper = vtk.vtkCompositePolyDataMapper()
sCutterMapper.SetInputConnection(sCutter.GetOutputPort())
sCutterMapper.ScalarVisibilityOff()

sCutterActor = vtk.vtkActor()
sCutterActor.SetMapper(sCutterMapper)
sCutterActor.GetProperty().SetColor(1,1,1)

outlineT = vtk.vtkOutlineFilter()
outlineT.SetInputData(input)

outlineMapperT = vtk.vtkPolyDataMapper()
outlineMapperT.SetInputConnection(outlineT.GetOutputPort())

outlineActorT = vtk.vtkActor()
outlineActorT.SetMapper(outlineMapperT)

# Time the execution of the filter w/out scalar tree
cutter_timer = vtk.vtkExecutionTimer()
cutter_timer.SetFilter(cutter)
cutter.Update()
CT = cutter_timer.GetElapsedWallClockTime()
print ("vtkCutter:", CT)

# Time the execution of the filter w/ sphere tree
sCutter_timer = vtk.vtkExecutionTimer()
sCutter_timer.SetFilter(sCutter)
sCutter.Update()
ST = sCutter_timer.GetElapsedWallClockTime()
print ("Build sphere tree + execute once:", ST)

sCutter_timer = vtk.vtkExecutionTimer()
sCutter_timer.SetFilter(sCutter)
plane.Modified()
sCutter.Update()
SC = sCutter_timer.GetElapsedWallClockTime()
print ("vtkPlaneCutter:", SC)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(outlineActor)
ren0.AddActor(cutterActor)
ren1.AddActor(outlineActorT)
ren1.AddActor(sCutterActor)

ren0.SetBackground(0,0,0)
ren1.SetBackground(0,0,0)
ren0.SetViewport(0,0,0.5,1);
ren1.SetViewport(0.5,0,1,1);
renWin.SetSize(600,300)
ren0.ResetCamera()
ren1.ResetCamera()
iren.Initialize()

renWin.Render()
#iren.Start()
# --- end of script --
