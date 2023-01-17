#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkMath
from vtkmodules.vtkCommonDataModel import (
    vtkPlane,
    vtkSphere,
)
from vtkmodules.vtkFiltersCore import vtkClipPolyData
from vtkmodules.vtkFiltersPoints import (
    vtkBoundedPointSource,
    vtkDensifyPointCloudFilter,
    vtkFitImplicitFunction,
    vtkPointDensityFilter,
)
from vtkmodules.vtkRenderingCore import (
    vtkImageSlice,
    vtkImageSliceMapper,
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
dens0.SetDensityFormToVolumeNormalized()
dens0.Update()
vrange = dens0.GetOutput().GetScalarRange()

map0 = vtkImageSliceMapper()
map0.BorderOn()
map0.SliceAtFocalPointOn()
map0.SliceFacesCameraOn()
map0.SetInputConnection(dens0.GetOutputPort())

slice0 = vtkImageSlice()
slice0.SetMapper(map0)
slice0.GetProperty().SetColorWindow(vrange[1]-vrange[0])
slice0.GetProperty().SetColorLevel(0.5*(vrange[0]+vrange[1]))

# Now densify the point cloud and reprocess
# Use relative radius
print("Number of input points: {0}".format(clipper.GetOutput().GetNumberOfPoints()))
denser = vtkDensifyPointCloudFilter()
denser.SetInputConnection(clipper.GetOutputPort())
denser.SetTargetDistance(0.025)
denser.SetMaximumNumberOfIterations(5)
denser.Update()
print("Number of output points: {0}".format(denser.GetOutput().GetNumberOfPoints()))

dens1 = vtkPointDensityFilter()
dens1.SetInputConnection(denser.GetOutputPort())
dens1.SetSampleDimensions(res,res,res)
dens1.SetDensityEstimateToFixedRadius()
dens1.SetRadius(0.05)
dens1.SetDensityFormToVolumeNormalized()
dens1.Update()
vrange = dens1.GetOutput().GetScalarRange()

map1 = vtkImageSliceMapper()
map1.BorderOn()
map1.SliceAtFocalPointOn()
map1.SliceFacesCameraOn()
map1.SetInputConnection(dens1.GetOutputPort())

slice1 = vtkImageSlice()
slice1.SetMapper(map1)
slice1.GetProperty().SetColorWindow(vrange[1]-vrange[0])
slice1.GetProperty().SetColorLevel(0.5*(vrange[0]+vrange[1]))

# Create the RenderWindow, Renderer and both Actors
#
ren0 = vtkRenderer()
ren0.SetViewport(0,0,0.5,1.0)
ren1 = vtkRenderer()
ren1.SetViewport(0.5,0,1,1.0)

renWin = vtkRenderWindow()
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(slice0)
ren0.SetBackground(0,0,0)
ren1.AddActor(slice1)
ren1.SetBackground(0,0,0)

renWin.SetSize(600,300)

cam = ren0.GetActiveCamera()
cam.ParallelProjectionOn()
cam.SetFocalPoint(0,0,0)
cam.SetPosition(0,0,1)
ren0.ResetCamera()

ren1.SetActiveCamera(cam)

iren.Initialize()

# render the image
#
renWin.Render()

iren.Start()
