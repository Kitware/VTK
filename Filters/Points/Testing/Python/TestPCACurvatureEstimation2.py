#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkMath
from vtkmodules.vtkCommonDataModel import (
    vtkBox,
    vtkCylinder,
    vtkImplicitBoolean,
    vtkSphere,
)
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkFiltersCore import vtkAssignAttribute
from vtkmodules.vtkFiltersExtraction import vtkExtractVectorComponents
from vtkmodules.vtkFiltersPoints import (
    vtkBoundedPointSource,
    vtkFitImplicitFunction,
    vtkPCACurvatureEstimation,
)
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPointGaussianMapper,
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
points.SetBounds(-3,3, -1,1, -1,1)
points.ProduceRandomScalarsOn()
points.ProduceCellOutputOff()
points.Update()

# Create a cylinder
cyl = vtkCylinder()
cyl.SetCenter(-2,0,0)
cyl.SetRadius(0.02)

# Create a (thin) box implicit function
box = vtkBox()
box.SetBounds(-1,0.5, -0.5,0.5, -0.0005, 0.0005)

# Create a sphere implicit function
sphere = vtkSphere()
sphere.SetCenter(2,0,0)
sphere.SetRadius(0.8)

# Boolean (union) these together
imp = vtkImplicitBoolean()
imp.SetOperationTypeToUnion()
imp.AddFunction(cyl)
imp.AddFunction(box)
imp.AddFunction(sphere)

# Extract points along sphere surface
extract = vtkFitImplicitFunction()
extract.SetInputConnection(points.GetOutputPort())
extract.SetImplicitFunction(imp)
extract.SetThreshold(0.0005)
extract.Update()

# Now generate normals from resulting points
curv = vtkPCACurvatureEstimation()
curv.SetInputConnection(extract.GetOutputPort())
curv.SetSampleSize(6)

# Time execution
timer = vtkTimerLog()
timer.StartTimer()
curv.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Points processed: {0}".format(NPts))
print("   Time to generate curvature: {0}".format(time))

# Break out the curvature into thress separate arrays
assign = vtkAssignAttribute()
assign.SetInputConnection(curv.GetOutputPort())
assign.Assign("PCACurvature", "VECTORS", "POINT_DATA")

extract = vtkExtractVectorComponents()
extract.SetInputConnection(assign.GetOutputPort())
extract.Update()
print(extract.GetOutput(0).GetScalarRange())
print(extract.GetOutput(1).GetScalarRange())
print(extract.GetOutput(2).GetScalarRange())

# Three different outputs for different curvatures
subMapper = vtkPointGaussianMapper()
subMapper.SetInputConnection(extract.GetOutputPort(0))
subMapper.EmissiveOff()
subMapper.SetScaleFactor(0.0)

subActor = vtkActor()
subActor.SetMapper(subMapper)
subActor.AddPosition(0,2.25,0)

sub1Mapper = vtkPointGaussianMapper()
sub1Mapper.SetInputConnection(extract.GetOutputPort(1))
sub1Mapper.EmissiveOff()
sub1Mapper.SetScaleFactor(0.0)

sub1Actor = vtkActor()
sub1Actor.SetMapper(sub1Mapper)

sub2Mapper = vtkPointGaussianMapper()
sub2Mapper.SetInputConnection(extract.GetOutputPort(2))
sub2Mapper.EmissiveOff()
sub2Mapper.SetScaleFactor(0.0)

sub2Actor = vtkActor()
sub2Actor.SetMapper(sub2Mapper)
sub2Actor.AddPosition(0,-2.25,0)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren0)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(subActor)
ren0.AddActor(sub1Actor)
ren0.AddActor(sub2Actor)
ren0.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(250,250)

cam = ren0.GetActiveCamera()
cam.SetFocalPoint(0,0,-1)
cam.SetPosition(0,0,0)
ren0.ResetCamera()

iren.Initialize()

# render the image
#
renWin.Render()

iren.Start()
