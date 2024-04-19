#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkMath
from vtkmodules.vtkCommonDataModel import (
    vtkImplicitBoolean,
    vtkPlane,
    vtkSphere,
)
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkFiltersPoints import (
    vtkBoundedPointSource,
    vtkExtractSurface,
    vtkFitImplicitFunction,
    vtkPCANormalEstimation,
    vtkSignedDistance,
)
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPointGaussianMapper,
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

# Interpolate onto a volume

# Parameters for debugging
NPts = 1000000
math = vtkMath()
math.RandomSeed(31415)

# create pipeline
#
points = vtkBoundedPointSource()
points.SetNumberOfPoints(NPts)
points.ProduceRandomScalarsOn()
points.ProduceCellOutputOff()
points.Update()

# Create a sphere implicit function
sphere = vtkSphere()
sphere.SetCenter(0,0,0)
sphere.SetRadius(0.75)

# Cut the sphere in half with a plane
plane = vtkPlane()
plane.SetOrigin(0,0,0)
plane.SetNormal(1,1,1)

# Boolean (intersect) these together to create a hemi-sphere
imp = vtkImplicitBoolean()
imp.SetOperationTypeToIntersection()
imp.AddFunction(sphere)
imp.AddFunction(plane)

# Extract points along hemi-sphere surface
extract = vtkFitImplicitFunction()
extract.SetInputConnection(points.GetOutputPort())
extract.SetImplicitFunction(imp)
extract.SetThreshold(0.005)
extract.Update()

# Now generate normals from resulting points
norms = vtkPCANormalEstimation()
norms.SetInputConnection(extract.GetOutputPort())
norms.SetSampleSize(20)
norms.FlipNormalsOff()
norms.SetNormalOrientationToGraphTraversal()
#norms.SetNormalOrientationToPoint()
#norms.SetOrientationPoint(0.3,0.3,0.3)
norms.Update()

subMapper = vtkPointGaussianMapper()
subMapper.SetInputConnection(extract.GetOutputPort())
subMapper.EmissiveOff()
subMapper.SetScaleFactor(0.0)

subActor = vtkActor()
subActor.SetMapper(subMapper)

# Generate signed distance function and contour it
dist = vtkSignedDistance()
dist.SetInputConnection(norms.GetOutputPort())
dist.SetRadius(0.1) #how far out to propagate distance calculation
dist.SetBounds(-1,1, -1,1, -1,1)
dist.SetDimensions(50,50,50)

# Extract the surface with modified flying edges
#fe = vtkFlyingEdges3D()
#fe.SetValue(0,0.0)
fe = vtkExtractSurface()
fe.SetInputConnection(dist.GetOutputPort())
fe.SetRadius(0.1) # this should match the signed distance radius

# Time the execution
timer = vtkTimerLog()
timer.StartTimer()
fe.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Points processed: {0}".format(NPts))
print("   Time to generate and extract distance function: {0}".format(time))
print("   Resulting bounds: {}".format(fe.GetOutput().GetBounds()))

feMapper = vtkPolyDataMapper()
feMapper.SetInputConnection(fe.GetOutputPort())

feActor = vtkActor()
feActor.SetMapper(feMapper)

# Create an outline
outline = vtkOutlineFilter()
outline.SetInputConnection(points.GetOutputPort())

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren0)
iren = vtkRenderWindowInteractor()
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

iren.Start()
