#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Control problem size and set debugging parameters
NPts = 1000
PointsPerBucket = 2

# create some points and display them
#
math = vtk.vtkMath()
math.RandomSeed(31415)
points = vtk.vtkPoints()
i = 0
while i < NPts:
    points.InsertPoint(i,math.Random(0,1),math.Random(0,2),math.Random(0,4))
    i = i + 1

# Specify hulls to view
HullId0 = 111

profile = vtk.vtkUnstructuredGrid()
profile.SetPoints(points)

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
voronoi.SetGenerateCellScalarsToPointIds()
voronoi.SetOutputTypeToBoundary()
voronoi.SetPointOfInterest(HullId0)

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

# Output the POI hull
surfMapper = vtk.vtkPolyDataMapper()
surfMapper.SetInputConnection(voronoi.GetOutputPort())
surfMapper.SetScalarRange(0,voronoi.GetOutput().GetNumberOfCells())
surfMapper.SetScalarModeToUseCellData()
print("Scalar Range: {}".format(surfMapper.GetScalarRange()))

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
#flowerMapper.ScalarVisibilityOff()

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
