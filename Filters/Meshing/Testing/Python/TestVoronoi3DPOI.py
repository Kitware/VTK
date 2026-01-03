#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Control problem size and set debugging parameters
NPts = 1000

# create some points and display them
#
math = vtk.vtkMath()
math.RandomSeed(31415)
points = vtk.vtkPoints()
i = 0
while i < NPts:
    points.InsertPoint(i,math.Random(0,1),math.Random(0,2),math.Random(0,4))
    i = i + 1

profile = vtk.vtkUnstructuredGrid()
profile.SetPoints(points)

# Tessellate them. Here we are only viewing one hull (the point of interest).
#
voronoi = vtk.vtkVoronoi3D()
voronoi.SetInputData(profile)
voronoi.SetGenerateCellScalarsToPrimIds()
voronoi.SetOutputTypeToVoronoi()
voronoi.SetPointOfInterest(100)

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

# Output the POI hull
surfMapper = vtk.vtkDataSetMapper()
surfMapper.SetInputConnection(voronoi.GetOutputPort())
surfMapper.SetScalarRange(0,voronoi.GetOutput().GetNumberOfCells())
surfMapper.SetScalarModeToUseCellData()
print("Scalar Range: {}".format(surfMapper.GetScalarRange()))
print("   Number of Voronoi cells produced: {}".format(voronoi.GetOutput().GetNumberOfCells()))

surfActor = vtk.vtkActor()
surfActor.SetMapper(surfMapper)
surfActor.GetProperty().SetColor(1,0,0)

# Output the Voronoi flower for this hull
ss = vtk.vtkSphereSource()
ss.SetRadius(1)
ss.SetCenter(0,0,0)
ss.SetThetaResolution(32)
ss.SetPhiResolution(16)

glypher = vtk.vtkGlyph3D()
glypher.SetInputConnection(voronoi.GetOutputPort())
glypher.SetSourceConnection(ss.GetOutputPort())
glypher.SetScaleModeToScaleByScalar()

flowerMapper = vtk.vtkPolyDataMapper()
flowerMapper.SetInputConnection(glypher.GetOutputPort())
flowerMapper.ScalarVisibilityOff()

flowerActor = vtk.vtkActor()
flowerActor.SetMapper(flowerMapper)
flowerActor.GetProperty().SetColor(0.5,0.5,0.5)
flowerActor.GetProperty().SetOpacity(0.075)

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
ren1.AddActor(flowerActor)

ren1.SetBackground(1,1,1)
renWin.SetSize(300,300)
renWin.Render()
cam1 = ren1.GetActiveCamera()

iren.Initialize()
# --- end of script --
