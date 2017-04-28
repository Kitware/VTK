#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Interpolate onto a volume

# Parameters for debugging
NPts = 1000000
math = vtk.vtkMath()
math.RandomSeed(31415)

# create pipeline
#
points = vtk.vtkBoundedPointSource()
points.SetNumberOfPoints(NPts)
points.ProduceRandomScalarsOn()
points.ProduceCellOutputOff()
points.Update()

# Create a sphere implicit function
sphere = vtk.vtkSphere()
sphere.SetCenter(0,0,0)
sphere.SetRadius(0.75)

# Extract points along sphere surface
extract = vtk.vtkFitImplicitFunction()
extract.SetInputConnection(points.GetOutputPort())
extract.SetImplicitFunction(sphere)
extract.SetThreshold(0.005)
extract.Update()

# Now generate normals from resulting points
norms = vtk.vtkPCANormalEstimation()
norms.SetInputConnection(extract.GetOutputPort())
norms.SetSampleSize(20)
norms.FlipNormalsOn()
norms.SetNormalOrientationToPoint()
norms.SetOrientationPoint(0,0,0)

# Time execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
norms.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Points processed: {0}".format(NPts))
print("   Time to generate normals: {0}".format(time))
#print(hBin)
#print(hBin.GetOutput())

subMapper = vtk.vtkPointGaussianMapper()
subMapper.SetInputConnection(norms.GetOutputPort())
subMapper.EmissiveOff()
subMapper.SetScaleFactor(0.0)

subActor = vtk.vtkActor()
subActor.SetMapper(subMapper)

# Draw the normals
mask = vtk.vtkMaskPoints()
mask.SetInputConnection(norms.GetOutputPort())
mask.SetRandomModeType(1)
mask.SetMaximumNumberOfPoints(250)

hhog = vtk.vtkHedgeHog()
hhog.SetInputConnection(mask.GetOutputPort())
hhog.SetVectorModeToUseNormal()
hhog.SetScaleFactor(0.25)

hogMapper = vtk.vtkPolyDataMapper()
hogMapper.SetInputConnection(hhog.GetOutputPort())

hogActor = vtk.vtkActor()
hogActor.SetMapper(hogMapper)

# Create an outline
outline = vtk.vtkOutlineFilter()
outline.SetInputConnection(points.GetOutputPort())

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren0)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(subActor)
ren0.AddActor(hogActor)
ren0.AddActor(outlineActor)
ren0.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(250,250)

cam = ren0.GetActiveCamera()
cam.SetFocalPoint(1,1,1)
cam.SetPosition(0,0,0)
ren0.ResetCamera()

iren.Initialize()

# render the image
#
renWin.Render()

iren.Start()
