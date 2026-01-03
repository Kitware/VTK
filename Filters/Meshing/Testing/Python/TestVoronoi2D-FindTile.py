#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkIdList,
    vtkMath,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import vtkPolyData
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkFiltersMeshing import vtkVoronoi2D
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPointGaussianMapper,
    vtkPointPicker,
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

# Control problem size and set debugging parameters
NPts = 500
PointOfInterest = 117
PointsPerBucket = 2

# create some points and display them
#
math = vtkMath()
math.RandomSeed(31415)
points = vtkPoints()
i = 0
while i < NPts:
    points.InsertPoint(i,math.Random(0,1),math.Random(0,1),0.0)
    i = i + 1

profile = vtkPolyData()
profile.SetPoints(points)

ptMapper = vtkPointGaussianMapper()
ptMapper.SetInputData(profile)
ptMapper.EmissiveOff()
ptMapper.SetScaleFactor(0.0)

ptActor = vtkActor()
ptActor.SetMapper(ptMapper)
ptActor.GetProperty().SetColor(0,0,0)
ptActor.GetProperty().SetPointSize(3)

# Tessellate them
#
voronoi = vtkVoronoi2D()
voronoi.SetInputData(profile)
voronoi.SetGenerateCellScalarsToPointIds()
voronoi.GetLocator().SetNumberOfPointsPerBucket(PointsPerBucket)
voronoi.SetOutputTypeToVoronoi()

# Query for tile
voronoi.Update()
x = [0,0,0]
points.GetPoint(PointOfInterest,x)
tileId = voronoi.FindTile(x)
print("FindTile: ", tileId)
tileData = vtkPolyData()
voronoi.GetTileData(tileId,tileData)

mapper = vtkPolyDataMapper()
mapper.SetInputConnection(voronoi.GetOutputPort())
mapper.SetScalarRange(0,NPts)
print("Scalar Range: {}".format(mapper.GetScalarRange()))

actor = vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetColor(1,0,0)

# Show the tile and query point
sphere = vtkSphereSource()
sphere.SetRadius(0.001)
sphere.SetThetaResolution(16)
sphere.SetPhiResolution(8)
sphere.SetCenter(points.GetPoint(PointOfInterest))

sphereMapper = vtkPolyDataMapper()
sphereMapper.SetInputConnection(sphere.GetOutputPort())

sphereActor = vtkActor()
sphereActor.SetMapper(sphereMapper)
sphereActor.GetProperty().SetColor(0,0,0)

tileMapper = vtkPolyDataMapper()
tileMapper.SetInputData(tileData)
tileMapper.SetScalarRange(0,NPts)

tileActor = vtkActor()
tileActor.SetMapper(tileMapper)
tileActor.GetProperty().SetColor(1,0,0)

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
ren1.SetViewport(0,0,0.5,1)
ren2 = vtkRenderer()
ren2.SetViewport(0.5,0,1,1)
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.AddActor(ptActor)
ren2.AddActor(sphereActor)
ren2.AddActor(tileActor)

ren1.SetBackground(1,1,1)
ren2.SetBackground(1,1,1)
renWin.SetSize(600,300)
renWin.Render()

iren.Initialize()
# --- end of script --
