#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Control problem size and set debugging parameters
NPts = 25
PointsPerBucket = 1

# create some points along a diagonal line
#
lineSource = vtk.vtkLineSource()
lineSource.SetPoint1(-1,-2,-3)
lineSource.SetPoint2( 1, 2, 3)
lineSource.SetResolution(NPts-1)
lineSource.Update()

output = lineSource.GetOutput()
outPts = output.GetPoints()
numPts = outPts.GetNumberOfPoints()
regionIds = vtk.vtkIntArray()
regionIds.SetName("RegionIds")
regionIds.SetNumberOfTuples(numPts)
for i in range(0, numPts):
    rid = i
    if i % 2 != 0:
        rid = -100
    regionIds.SetComponent(i,0, rid)
output.GetPointData().SetScalars(regionIds)

# Tessellate them
#
voronoi = vtk.vtkVoronoiFlower3D()
voronoi.SetInputData(output)
voronoi.SetPadding(0.01)
voronoi.SetGenerateCellScalarsToPointIds()
voronoi.SetOutputTypeToBoundary()

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
print("Scalar Range: {}".format(surfMapper.GetScalarRange()))
print("   Number of primitives produced: {}".format(surfMapper.GetInput().GetPolys().GetNumberOfCells()))

surfActor = vtk.vtkActor()
surfActor.SetMapper(surfMapper)
surfActor.GetProperty().SetColor(1,0,0)

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

iren.Initialize()
# --- end of script --
