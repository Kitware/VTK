#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

res = 50

# Create the RenderWindow, Renderer and both Actors
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
sphere.SetCenter( 0.0,0.0,0.0)
sphere.SetRadius(0.25)

sample = vtk.vtkSampleFunction()
sample.SetImplicitFunction(sphere)
sample.SetModelBounds(-0.5,0.5, -0.5,0.5, -0.5,0.5)
sample.SetSampleDimensions(res,res,res)
sample.Update()

# Convert the image data to unstructured grid
extractionSphere = vtk.vtkSphere()
extractionSphere.SetRadius(100)
extractionSphere.SetCenter(0,0,0)
extract = vtk.vtkExtractGeometry()
extract.SetImplicitFunction(extractionSphere)
extract.SetInputConnection(sample.GetOutputPort())
extract.Update()

# The cut plane
plane = vtk.vtkPlane()
plane.SetOrigin(0,0,0)
plane.SetNormal(1,1,1)

# Now create the usual cutter
cutter = vtk.vtkCutter()
cutter.SetInputConnection(extract.GetOutputPort())
cutter.SetCutFunction(plane)

cutterMapper = vtk.vtkPolyDataMapper()
cutterMapper.SetInputConnection(cutter.GetOutputPort())
cutterMapper.ScalarVisibilityOff()

cutterActor = vtk.vtkActor()
cutterActor.SetMapper(cutterMapper)
cutterActor.GetProperty().SetColor(1,1,1)

# Throw in an outline
outline = vtk.vtkOutlineFilter()
outline.SetInputConnection(sample.GetOutputPort())

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)

# Accelerated cutter
cut = vtk.vtkPlaneCutter()
cut.SetInputConnection(extract.GetOutputPort())
cut.SetPlane(plane)
cut.ComputeNormalsOff()

sCutterMapper = vtk.vtkCompositePolyDataMapper()
sCutterMapper.SetInputConnection(cut.GetOutputPort())
sCutterMapper.ScalarVisibilityOff()

sCutterActor = vtk.vtkActor()
sCutterActor.SetMapper(sCutterMapper)
sCutterActor.GetProperty().SetColor(1,1,1)

outlineT = vtk.vtkOutlineFilter()
outlineT.SetInputConnection(sample.GetOutputPort())

outlineMapperT = vtk.vtkPolyDataMapper()
outlineMapperT.SetInputConnection(outlineT.GetOutputPort())

outlineActorT = vtk.vtkActor()
outlineActorT.SetMapper(outlineMapperT)

# Time the execution of the usual cutter
cutter_timer = vtk.vtkExecutionTimer()
cutter_timer.SetFilter(cutter)
cutter.Update()
CT = cutter_timer.GetElapsedWallClockTime()
print ("vtkCutter:", CT)

# Time the execution of the filter w/ sphere tree
sCutter_timer = vtk.vtkExecutionTimer()
sCutter_timer.SetFilter(cut)
cut.Update()
ST = sCutter_timer.GetElapsedWallClockTime()
print ("Build sphere tree + execute once:", ST)

# Time subsequent cuts
sCutter_timer.SetFilter(cut)
plane.Modified()
cut.Update()
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
