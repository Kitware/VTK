#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

import sys

# Control problem size and set debugging parameters. For VTK
# testing (ctest), a default value is used. Otherwise, users can
# manually run the test with a specified number of points.
NPts = 100
if len(sys.argv) > 1:
    try:
        NPts = int(sys.argv[1])
    except ValueError:
        NPts = NPts

PointsPerBucket = 1

# Create the RenderWindow, Renderer and Actors
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
voronoi = vtk.vtkVoronoi3D()
voronoi.SetInputData(profile)
voronoi.SetPadding(0.001)
voronoi.SetGenerateCellScalarsToRandom()
voronoi.GetLocator().SetNumberOfPointsPerBucket(PointsPerBucket)
voronoi.SetOutputTypeToDelaunay()
voronoi.SetBatchSize(1000)
voronoi.PassPointDataOn()
#voronoi.ValidateOn()

# Time execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
voronoi.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Number of points processed: {0}".format(NPts))
print("   Time to generate Delaunay tetrahedralization: {0}".format(time))
print("   Number of threads used: {0}".format(voronoi.GetNumberOfThreadsUsed()))
print("   Number of tets produced: {0}".format(voronoi.GetOutput().GetNumberOfCells()))

volMapper = vtk.vtkDataSetMapper()
volMapper.SetInputConnection(voronoi.GetOutputPort())
volMapper.SetScalarRange(0,64)
volMapper.SetScalarModeToUseCellData()

# support picking (for debugging)
scalars = voronoi.GetOutput().GetCellData().GetArray("Voronoi Cell Scalars")

tetActor = vtk.vtkActor()
tetActor.SetMapper(volMapper)
tetActor.GetProperty().SetColor(1,0,0)
tetActor.GetProperty().EdgeVisibilityOn()

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(tetActor)

ren1.SetBackground(1,1,1)
renWin.SetSize(300,300)
renWin.Render()
cam1 = ren1.GetActiveCamera()

# For debugging: select point ids
def reportPointId(obj=None, event=""):
    print("Hull Id: {}".format(scalars.GetValue(picker.GetCellId())))

picker = vtk.vtkCellPicker()
picker.AddObserver("EndPickEvent",reportPointId)
picker.PickFromListOn()
picker.AddPickList(tetActor)
iren.SetPicker(picker)

iren.Initialize()
iren.Start()
# --- end of script --
