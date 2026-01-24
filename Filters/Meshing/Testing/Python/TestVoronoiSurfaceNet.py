#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Control problem size and set debugging parameters
NPts = 50
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
    regionIds.SetComponent(i,0, i+1)
output.GetPointData().SetScalars(regionIds)

# Extract the surface net regions
#
vsn = vtk.vtkGeneralizedSurfaceNets3D()
vsn.SetInputData(output)
vsn.SetBackgroundLabel(-100)
vsn.SmoothingOff()
num = 0
for region in range(0, numPts):
    if region % 2 != 0:
        vsn.SetLabel(num,region+1)
        num += 1

# Time execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
vsn.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Number of points processed: {0}".format(NPts))
print("   Time to generate Voronoi Surface Net tessellation: {0}".format(time))
print("   Number of threads used: {0}".format(vsn.GetNumberOfThreads()))

numCells = vsn.GetOutput().GetNumberOfCells()
for cid in range(0, numCells):
    print(vsn.GetOutput().GetCellData().GetScalars().GetTuple(cid))

surfMapper = vtk.vtkPolyDataMapper()
surfMapper.SetInputConnection(vsn.GetOutputPort())
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
