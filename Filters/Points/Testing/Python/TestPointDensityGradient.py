#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkMath
from vtkmodules.vtkCommonDataModel import (
    vtkPlane,
    vtkSphere,
)
from vtkmodules.vtkFiltersCore import (
    vtkAssignAttribute,
    vtkClipPolyData,
    vtkFlyingEdgesPlaneCutter,
    vtkHedgeHog,
)
from vtkmodules.vtkFiltersPoints import (
    vtkBoundedPointSource,
    vtkFitImplicitFunction,
    vtkPointDensityFilter,
)
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkImageSlice,
    vtkImageSliceMapper,
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

# The resolution of the density function volume
res = 100

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
sphere.SetCenter(0.0,0.1,0.2)
sphere.SetRadius(0.75)

# Extract points within sphere
extract = vtkFitImplicitFunction()
extract.SetInputConnection(points.GetOutputPort())
extract.SetImplicitFunction(sphere)
extract.SetThreshold(0.005)
extract.GenerateVerticesOn()

# Clip out some of the points with a plane; requires vertices
plane = vtkPlane()
plane.SetOrigin(sphere.GetCenter())
plane.SetNormal(1,1,1)

clipper = vtkClipPolyData()
clipper.SetInputConnection(extract.GetOutputPort())
clipper.SetClipFunction(plane);

# Generate density field from points
# Use fixed radius
dens0 = vtkPointDensityFilter()
dens0.SetInputConnection(clipper.GetOutputPort())
dens0.SetSampleDimensions(res,res,res)
dens0.SetDensityEstimateToFixedRadius()
dens0.SetRadius(0.05)
#dens0.SetDensityEstimateToRelativeRadius()
dens0.SetRelativeRadius(2.5)
#dens0.SetDensityFormToVolumeNormalized()
dens0.SetDensityFormToNumberOfPoints()
dens0.ComputeGradientOn()
dens0.Update()
vrange = dens0.GetOutput().GetPointData().GetArray("Gradient Magnitude").GetRange()

# Show the gradient magnitude
assign0 = vtkAssignAttribute()
assign0.SetInputConnection(dens0.GetOutputPort())
assign0.Assign("Gradient Magnitude", "SCALARS", "POINT_DATA")

map0 = vtkImageSliceMapper()
map0.BorderOn()
map0.SliceAtFocalPointOn()
map0.SliceFacesCameraOn()
map0.SetInputConnection(assign0.GetOutputPort())

slice0 = vtkImageSlice()
slice0.SetMapper(map0)
slice0.GetProperty().SetColorWindow(vrange[1]-vrange[0])
slice0.GetProperty().SetColorLevel(0.5*(vrange[0]+vrange[1]))

# Show the region labels
assign1 = vtkAssignAttribute()
assign1.SetInputConnection(dens0.GetOutputPort())
assign1.Assign("Classification", "SCALARS", "POINT_DATA")

map1 = vtkImageSliceMapper()
map1.BorderOn()
map1.SliceAtFocalPointOn()
map1.SliceFacesCameraOn()
map1.SetInputConnection(assign1.GetOutputPort())
map1.Update()

slice1 = vtkImageSlice()
slice1.SetMapper(map1)
slice1.GetProperty().SetColorWindow(1)
slice1.GetProperty().SetColorLevel(0.5)

# Show the vectors (gradient)
assign2 = vtkAssignAttribute()
assign2.SetInputConnection(dens0.GetOutputPort())
assign2.Assign("Gradient", "VECTORS", "POINT_DATA")

plane = vtkPlane()
plane.SetNormal(0,0,1)
plane.SetOrigin(0.0701652, 0.172689, 0.27271)

cut = vtkFlyingEdgesPlaneCutter()
cut.SetInputConnection(assign2.GetOutputPort())
cut.SetPlane(plane)
cut.InterpolateAttributesOn()

v = vtkHedgeHog()
v.SetInputConnection(cut.GetOutputPort())
v.SetScaleFactor(0.0001)

vMapper = vtkPolyDataMapper()
vMapper.SetInputConnection(v.GetOutputPort())
vMapper.SetScalarRange(vrange)

vectors = vtkActor()
vectors.SetMapper(vMapper)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = vtkRenderer()
ren0.SetViewport(0,0,0.333,1)
ren1 = vtkRenderer()
ren1.SetViewport(0.333,0,0.667,1)
ren2 = vtkRenderer()
ren2.SetViewport(0.667,0,1,1)

renWin = vtkRenderWindow()
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(slice0)
ren0.SetBackground(0,0,0)
ren1.AddActor(slice1)
ren1.SetBackground(0,0,0)
ren2.AddActor(vectors)
ren2.SetBackground(0,0,0)

renWin.SetSize(450,150)

cam = ren0.GetActiveCamera()
cam.ParallelProjectionOn()
cam.SetFocalPoint(0,0,0)
cam.SetPosition(0,0,1)
ren0.ResetCamera()

ren1.SetActiveCamera(cam)
ren2.SetActiveCamera(cam)

iren.Initialize()

# render the image
#
renWin.Render()

iren.Start()
