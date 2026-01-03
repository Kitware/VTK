#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

import sys

# Control problem size and set debugging parameters
NPts = 1000
if len(sys.argv) > 1:
    try:
        NPts = int(sys.argv[1])
    except ValueError:
        NPts = NPts

PointsPerBucket = 1

# create some points on a Lissajous curve
#
lissa = vtk.vtkLissajousPointCloud()
lissa.SetNumberOfPoints(NPts)
lissa.DeterministicNoiseOn()
lissa.Update()

# Tessellate them
#
voronoi = vtk.vtkVoronoi3D()
voronoi.SetInputConnection(lissa.GetOutputPort())
voronoi.SetPadding(0.001)
voronoi.GetLocator().SetNumberOfPointsPerBucket(PointsPerBucket)
voronoi.SetGenerateCellScalarsToRandom()
voronoi.SetOutputTypeToBoundary()
voronoi.ValidateOn()

# Time execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
voronoi.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Number of points processed: {0}".format(NPts))
print("   Time to generate Voronoi tessellation: {0}".format(time))
print("   Number of threads used: {0}".format(voronoi.GetNumberOfThreadsUsed()))
print("   Max number of points in any hull: {0}".format(voronoi.GetMaximumNumberOfPoints()))
print("   Max number of faces in any hull: {0}".format(voronoi.GetMaximumNumberOfFaces()))
print("   Points generated: {0}".format(voronoi.GetOutput().GetNumberOfPoints()))
print("   Cells generated: {0}".format(voronoi.GetOutput().GetNumberOfCells()))
print("   Number of prunes: {0}".format(voronoi.GetNumberOfPrunes()))

surfMapper = vtk.vtkPolyDataMapper()
surfMapper.SetInputConnection(voronoi.GetOutputPort())
surfMapper.SetScalarRange(0,64)
surfMapper.SetScalarModeToUseCellData()
print("Scalar Range: {}".format(surfMapper.GetScalarRange()))
print("   Number of primitives produced: {}".format(voronoi.GetOutput().GetNumberOfCells()))

surfActor = vtk.vtkActor()
surfActor.SetMapper(surfMapper)
surfActor.GetProperty().SetColor(1,0,0)
surfActor.GetProperty().EdgeVisibilityOn()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(surfActor)
ren1.SetBackground(1,1,1)
renWin.SetSize(300,300)
renWin.Render()
cam1 = ren1.GetActiveCamera()
cam1.SetPosition(1,0,0)
ren1.ResetCamera()

renWin.Initialize()
iren.Start()
# --- end of script --
