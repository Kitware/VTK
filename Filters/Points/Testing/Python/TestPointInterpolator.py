#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Parameters for debugging
res = 200

# create pipeline
#
pl3d = vtk.vtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(VTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()

output = pl3d.GetOutput().GetBlock(0)

# Create a probe plane
center = output.GetCenter()

plane = vtk.vtkPlaneSource()
plane.SetResolution(res,res)
plane.SetOrigin(0,0,0)
plane.SetPoint1(10,0,0)
plane.SetPoint2(0,10,0)
plane.SetCenter(center)
plane.SetNormal(0,1,0)

# Reuse the locator
locator = vtk.vtkStaticPointLocator()
locator.SetDataSet(output)
locator.BuildLocator()

# Voronoi kernel------------------------------------------------
voronoiKernel = vtk.vtkVoronoiKernel()

interpolator = vtk.vtkPointInterpolator()
interpolator.SetInputConnection(plane.GetOutputPort())
interpolator.SetSourceData(output)
interpolator.SetKernel(voronoiKernel)
interpolator.SetLocator(locator)

# Time execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
interpolator.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Interpolate Points (Voronoi): {0}".format(time))

intMapper = vtk.vtkPolyDataMapper()
intMapper.SetInputConnection(interpolator.GetOutputPort())

intActor = vtk.vtkActor()
intActor.SetMapper(intMapper)

# Create an outline
outline = vtk.vtkStructuredGridOutlineFilter()
outline.SetInputData(output)

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)

# Gaussian kernel-------------------------------------------------------
gaussianKernel = vtk.vtkGaussianKernel()
#gaussianKernel = vtk.vtkEllipsoidalGaussianKernel()
#gaussianKernel.UseScalarsOn()
#gaussianKernel.UseNormalsOn()
gaussianKernel.SetSharpness(4)
gaussianKernel.SetRadius(0.5)

interpolator1 = vtk.vtkPointInterpolator()
interpolator1.SetInputConnection(plane.GetOutputPort())
interpolator1.SetSourceData(output)
interpolator1.SetKernel(gaussianKernel)
interpolator1.SetLocator(locator)
interpolator1.SetNullPointsStrategyToNullValue()

# Time execution
timer.StartTimer()
interpolator1.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Interpolate Points (Gaussian): {0}".format(time))

intMapper1 = vtk.vtkPolyDataMapper()
intMapper1.SetInputConnection(interpolator1.GetOutputPort())

intActor1 = vtk.vtkActor()
intActor1.SetMapper(intMapper1)

# Create an outline
outline1 = vtk.vtkStructuredGridOutlineFilter()
outline1.SetInputData(output)

outlineMapper1 = vtk.vtkPolyDataMapper()
outlineMapper1.SetInputConnection(outline1.GetOutputPort())

outlineActor1 = vtk.vtkActor()
outlineActor1.SetMapper(outlineMapper1)

# Shepard kernel-------------------------------------------------------
shepardKernel = vtk.vtkShepardKernel()
shepardKernel.SetPowerParameter(2)
shepardKernel.SetRadius(0.5)

interpolator2 = vtk.vtkPointInterpolator()
interpolator2.SetInputConnection(plane.GetOutputPort())
interpolator2.SetSourceData(output)
interpolator2.SetKernel(shepardKernel)
interpolator2.SetLocator(locator)
interpolator2.SetNullPointsStrategyToMaskPoints()

# Time execution
timer.StartTimer()
interpolator2.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Interpolate Points (Shepard): {0}".format(time))

intMapper2 = vtk.vtkPolyDataMapper()
intMapper2.SetInputConnection(interpolator2.GetOutputPort())

intActor2 = vtk.vtkActor()
intActor2.SetMapper(intMapper2)

# Create an outline
outline2 = vtk.vtkStructuredGridOutlineFilter()
outline2.SetInputData(output)

outlineMapper2 = vtk.vtkPolyDataMapper()
outlineMapper2.SetInputConnection(outline2.GetOutputPort())

outlineActor2 = vtk.vtkActor()
outlineActor2.SetMapper(outlineMapper2)

# Linear kernel-------------------------------------------------------
linearKernel = vtk.vtkLinearKernel()
linearKernel.SetRadius(0.5)

interpolator3 = vtk.vtkPointInterpolator()
interpolator3.SetInputConnection(plane.GetOutputPort())
interpolator3.SetSourceData(output)
interpolator3.SetKernel(linearKernel)
interpolator3.SetLocator(locator)
interpolator3.SetNullPointsStrategyToNullValue()
interpolator3.AddExcludedArray("StagnationEnergy")

# Time execution
timer.StartTimer()
interpolator3.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Interpolate Points (Linear): {0}".format(time))

intMapper3 = vtk.vtkPolyDataMapper()
intMapper3.SetInputConnection(interpolator3.GetOutputPort())

intActor3 = vtk.vtkActor()
intActor3.SetMapper(intMapper3)

# Create an outline
outline3 = vtk.vtkStructuredGridOutlineFilter()
outline3.SetInputData(output)

outlineMapper3 = vtk.vtkPolyDataMapper()
outlineMapper3.SetInputConnection(outline3.GetOutputPort())

outlineActor3 = vtk.vtkActor()
outlineActor3.SetMapper(outlineMapper3)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = vtk.vtkRenderer()
ren0.SetViewport(0,0,.5,.5)
ren1 = vtk.vtkRenderer()
ren1.SetViewport(0.5,0,1,.5)
ren2 = vtk.vtkRenderer()
ren2.SetViewport(0,0.5,.5,1)
ren3 = vtk.vtkRenderer()
ren3.SetViewport(0.5,0.5,1,1)
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.AddRenderer(ren3)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(intActor)
ren0.AddActor(outlineActor)
ren0.SetBackground(0.1, 0.2, 0.4)

ren1.AddActor(intActor1)
ren1.AddActor(outlineActor1)
ren1.SetBackground(0.1, 0.2, 0.4)

ren2.AddActor(intActor2)
ren2.AddActor(outlineActor2)
ren2.SetBackground(0.1, 0.2, 0.4)

ren3.AddActor(intActor3)
ren3.AddActor(outlineActor3)
ren3.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(500, 500)

cam = ren0.GetActiveCamera()
cam.SetClippingRange(3.95297, 50)
cam.SetFocalPoint(8.88908, 0.595038, 29.3342)
cam.SetPosition(-12.3332, 31.7479, 41.2387)
cam.SetViewUp(0.060772, -0.319905, 0.945498)

ren1.SetActiveCamera(cam)
ren2.SetActiveCamera(cam)
ren3.SetActiveCamera(cam)

iren.Initialize()

# render the image
#
renWin.Render()

iren.Start()
