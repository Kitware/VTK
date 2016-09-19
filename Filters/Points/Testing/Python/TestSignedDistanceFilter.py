#!/usr/bin/env python
import vtk
from vtk.test import Testing
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

# Cut the sphere in half with a plane
plane = vtk.vtkPlane()
plane.SetOrigin(0,0,0)
plane.SetNormal(1,1,1)

# Boolean (intersect) these together to create a hemi-sphere
imp = vtk.vtkImplicitBoolean()
imp.SetOperationTypeToIntersection()
imp.AddFunction(sphere)
imp.AddFunction(plane)

# Extract points along hemi-sphere surface
extract = vtk.vtkFitImplicitFunction()
extract.SetInputConnection(points.GetOutputPort())
extract.SetImplicitFunction(imp)
extract.SetThreshold(0.005)
extract.Update()

# Now generate normals from resulting points
norms = vtk.vtkPCANormalEstimation()
norms.SetInputConnection(extract.GetOutputPort())
norms.SetSampleSize(20)
norms.FlipNormalsOff()
norms.SetNormalOrientationToGraphTraversal()
#norms.SetNormalOrientationToPoint()
#norms.SetOrientationPoint(0.3,0.3,0.3)
norms.Update()

subMapper = vtk.vtkPointGaussianMapper()
subMapper.SetInputConnection(extract.GetOutputPort())
subMapper.EmissiveOff()
subMapper.SetScaleFactor(0.0)

subActor = vtk.vtkActor()
subActor.SetMapper(subMapper)

# Generate signed distance function and contour it
dist = vtk.vtkSignedDistance()
dist.SetInputConnection(norms.GetOutputPort())
dist.SetRadius(0.1) #how far out to propagate distance calculation
dist.SetBounds(-1,1, -1,1, -1,1)
dist.SetDimensions(50,50,50)

# Extract the surface with modified flying edges
fe = vtk.vtkExtractSurface()
fe.SetInputConnection(dist.GetOutputPort())
fe.SetRadius(0.1) # this should match the signed distance radius

# Time the execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
fe.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Points processed: {0}".format(NPts))
print("   Time to generate and extract distance function: {0}".format(time))

feMapper = vtk.vtkPolyDataMapper()
feMapper.SetInputConnection(fe.GetOutputPort())

feActor = vtk.vtkActor()
feActor.SetMapper(feMapper)

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
ren0.AddActor(feActor)
ren0.AddActor(outlineActor)
ren0.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(250,250)

cam = ren0.GetActiveCamera()
cam.SetFocalPoint(1,-1,-1)
cam.SetPosition(0,0,0)
ren0.ResetCamera()

iren.Initialize()

# render the image
#
renWin.Render()

#iren.Start()
