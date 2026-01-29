#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

import sys

# Control problem size and set debugging parameters. For VTK
# testing (ctest), a default value is used. Otherwise, users can
# manually run the test with a specified number of points.
NPts = 1000
if len(sys.argv) > 1:
    try:
        NPts = int(sys.argv[1])
    except ValueError:
        NPts = NPts

PointsPerBucket = 1

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create some points and display them
#
math = vtk.vtkMath()
math.RandomSeed(31415)
points = vtk.vtkPoints()
points.SetNumberOfPoints(NPts)
ids = vtk.vtkIdTypeArray()
ids.SetName("Point Ids")
ids.SetNumberOfTuples(NPts)
i = 0
while i < NPts:
    points.SetPoint(i,math.Random(0,1),math.Random(0,2),math.Random(0,4))
    ids.SetValue(i,i)
    i = i + 1

profile = vtk.vtkUnstructuredGrid()
profile.SetPoints(points)
profile.GetPointData().AddArray(ids)

ptMapper = vtk.vtkDataSetMapper()
ptMapper.SetInputData(profile)

ptActor = vtk.vtkActor()
ptActor.SetMapper(ptMapper)
ptActor.GetProperty().SetColor(0,0,0)
ptActor.GetProperty().SetPointSize(2)

# Tessellate them
#
voronoi = vtk.vtkVoronoiFlower3D()
voronoi.SetInputData(profile)
voronoi.SetPadding(0.005)
voronoi.SetGenerateCellScalarsToNone()
voronoi.GetLocator().SetNumberOfPointsPerBucket(PointsPerBucket)
voronoi.SetOutputTypeToAdjacencyGraph()
voronoi.PassPointDataOn()

# Time execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
voronoi.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Number of points processed: {0}".format(NPts))
print("   Time to generate Voronoi tessellation: {0}".format(time))
print("   Number of threads used: {0}".format(voronoi.GetNumberOfThreads()))
print("   Max number of points in any hull: {0}".format(voronoi.GetMaximumNumberOfPoints()))
print("   Max number of faces in any hull: {0}".format(voronoi.GetMaximumNumberOfFaces()))

surfMapper = vtk.vtkPolyDataMapper()
surfMapper.SetInputConnection(voronoi.GetOutputPort())
surfMapper.SetScalarRange(0,NPts)
surfMapper.SetScalarModeToUsePointFieldData()
surfMapper.SelectColorArray("Point Ids")
print("Scalar Range: {}".format(surfMapper.GetScalarRange()))
print("   Number of points produced: {}".format(surfMapper.GetInput().GetNumberOfPoints()))
print("   Number of primitives produced: {}".format(surfMapper.GetInput().GetLines().GetNumberOfCells()))

surfActor = vtk.vtkActor()
surfActor.SetMapper(surfMapper)
surfActor.GetProperty().SetColor(1,0,0)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(surfActor)

ren1.SetBackground(1,1,1)
renWin.SetSize(300,300)
renWin.Render()
cam1 = ren1.GetActiveCamera()

iren.Initialize()
# --- end of script --
