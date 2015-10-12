#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

useFECutter = 1
res = 100

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
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

# The cut plane
plane = vtk.vtkPlane()
plane.SetOrigin(0,0,0)
plane.SetNormal(1,1,1)

if useFECutter:
    cut = vtk.vtkFlyingEdgesPlaneCutter()
    cut.SetInputConnection(sample.GetOutputPort())
    cut.SetPlane(plane)
    cut.ComputeNormalsOff() #make it equivalent to vtkCutter
else:
    # Compare against previous method
    cut = vtk.vtkCutter()
    cut.SetInputConnection(sample.GetOutputPort())
    cut.SetCutFunction(plane)
    cut.SetValue(0,0.0)

# Time the execution of the filter w/out scalar tree
CG_timer = vtk.vtkExecutionTimer()
CG_timer.SetFilter(cut)
cut.Update()
CG = CG_timer.GetElapsedWallClockTime()
print ("Cut volume:", CG)

cutMapper = vtk.vtkPolyDataMapper()
cutMapper.SetInputConnection(cut.GetOutputPort())

cutActor = vtk.vtkActor()
cutActor.SetMapper(cutMapper)
cutActor.GetProperty().SetColor(1,1,1)
cutActor.GetProperty().SetOpacity(1)

outline = vtk.vtkOutlineFilter()
outline.SetInputConnection(sample.GetOutputPort())

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp = outlineActor.GetProperty()

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(outlineActor)
ren1.AddActor(cutActor)
ren1.SetBackground(0,0,0)
renWin.SetSize(400,400)
ren1.ResetCamera()
iren.Initialize()

renWin.Render()
# --- end of script --
#iren.Start()
