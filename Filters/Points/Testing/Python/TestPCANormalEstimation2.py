#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkMath
from vtkmodules.vtkCommonDataModel import vtkSphere
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkFiltersCore import (
    vtkHedgeHog,
    vtkMaskPoints,
)
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkFiltersPoints import (
    vtkBoundedPointSource,
    vtkFitImplicitFunction,
    vtkPCANormalEstimation,
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

# Extract points along sphere surface
extract = vtkFitImplicitFunction()
extract.SetInputConnection(points.GetOutputPort())
extract.SetImplicitFunction(sphere)
extract.SetThreshold(0.005)
extract.Update()

# Now generate normals from resulting points
norms = vtkPCANormalEstimation()
norms.SetInputConnection(extract.GetOutputPort())
norms.SetSampleSize(20)
norms.FlipNormalsOn()
norms.SetNormalOrientationToGraphTraversal()

# Time execution
timer = vtkTimerLog()
timer.StartTimer()
norms.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Points processed: {0}".format(NPts))
print("   Time to generate normals: {0}".format(time))
#print(hBin)
#print(hBin.GetOutput())

subMapper = vtkPointGaussianMapper()
subMapper.SetInputConnection(norms.GetOutputPort())
subMapper.EmissiveOff()
subMapper.SetScaleFactor(0.0)

subActor = vtkActor()
subActor.SetMapper(subMapper)

# Draw the normals
mask = vtkMaskPoints()
mask.SetInputConnection(norms.GetOutputPort())
mask.SetRandomModeType(1)
mask.SetMaximumNumberOfPoints(250)

hhog = vtkHedgeHog()
hhog.SetInputConnection(mask.GetOutputPort())
hhog.SetVectorModeToUseNormal()
hhog.SetScaleFactor(0.25)

hogMapper = vtkPolyDataMapper()
hogMapper.SetInputConnection(hhog.GetOutputPort())

hogActor = vtkActor()
hogActor.SetMapper(hogMapper)

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
