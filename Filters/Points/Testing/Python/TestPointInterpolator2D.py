#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Parameters for debugging
NPts = 1000000
math = vtk.vtkMath()

# create pipeline: use terrain dataset
#
# Read the data: a height field results
demReader = vtk.vtkDEMReader()
demReader.SetFileName(VTK_DATA_ROOT + "/Data/SainteHelens.dem")
demReader.Update()

lo = demReader.GetOutput().GetScalarRange()[0]
hi = demReader.GetOutput().GetScalarRange()[1]

geom = vtk.vtkImageDataGeometryFilter()
geom.SetInputConnection(demReader.GetOutputPort())

warp = vtk.vtkWarpScalar()
warp.SetInputConnection(geom.GetOutputPort())
warp.SetNormal(0, 0, 1)
warp.UseNormalOn()
warp.SetScaleFactor(2)
warp.Update()

bds = warp.GetOutput().GetBounds()
center = warp.GetOutput().GetCenter()

# A randomized point cloud, whose attributes are set via implicit function
points = vtk.vtkPoints()
points.SetDataTypeToFloat()
points.SetNumberOfPoints(NPts)
for i in range(0,NPts):
    points.SetPoint(i,math.Random(bds[0],bds[1]),math.Random(bds[2],bds[3]),math.Random(bds[4],bds[5]))

source = vtk.vtkPolyData()
source.SetPoints(points)

sphere = vtk.vtkSphere()
sphere.SetCenter(center[0],center[1]-7500,center[2])

attr = vtk.vtkSampleImplicitFunctionFilter()
attr.SetInputData(source)
attr.SetImplicitFunction(sphere)
attr.Update()

# Gaussian kernel-------------------------------------------------------
gaussianKernel = vtk.vtkGaussianKernel()
gaussianKernel.SetSharpness(4)
gaussianKernel.SetRadius(50)

voronoiKernel = vtk.vtkVoronoiKernel()

interpolator1 = vtk.vtkPointInterpolator2D()
interpolator1.SetInputConnection(warp.GetOutputPort())
interpolator1.SetSourceConnection(attr.GetOutputPort())
#interpolator1.SetKernel(gaussianKernel)
interpolator1.SetKernel(voronoiKernel)
interpolator1.SetNullPointsStrategyToClosestPoint()

# Time execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
interpolator1.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Interpolate Terrain Points (Gaussian): {0}".format(time))

scalarRange = attr.GetOutput().GetScalarRange()

intMapper1 = vtk.vtkPolyDataMapper()
intMapper1.SetInputConnection(interpolator1.GetOutputPort())
intMapper1.SetScalarRange(scalarRange)

intActor1 = vtk.vtkActor()
intActor1.SetMapper(intMapper1)

# Create an outline
outline1 = vtk.vtkOutlineFilter()
outline1.SetInputConnection(warp.GetOutputPort())

outlineMapper1 = vtk.vtkPolyDataMapper()
outlineMapper1.SetInputConnection(outline1.GetOutputPort())

outlineActor1 = vtk.vtkActor()
outlineActor1.SetMapper(outlineMapper1)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren0)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(intActor1)
ren0.AddActor(outlineActor1)
ren0.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(250, 250)

cam = ren0.GetActiveCamera()
cam.SetFocalPoint(center)

fp = cam.GetFocalPoint()
cam.SetPosition(fp[0]+.2,fp[1]+.1,fp[2]+1)
ren0.ResetCamera()

iren.Initialize()

# render the image
#
renWin.Render()

iren.Start()
