#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import vtkStaticPointLocator
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkFiltersCore import vtkStructuredGridOutlineFilter
from vtkmodules.vtkFiltersPoints import (
    vtkGaussianKernel,
    vtkLinearKernel,
    vtkPointInterpolator,
    vtkShepardKernel,
    vtkVoronoiKernel,
)
from vtkmodules.vtkFiltersSources import vtkPlaneSource
from vtkmodules.vtkIOParallel import vtkMultiBlockPLOT3DReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
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

# Parameters for debugging
res = 200

# create pipeline
#
pl3d = vtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(VTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()

output = pl3d.GetOutput().GetBlock(0)

# Create a probe plane
center = output.GetCenter()

plane = vtkPlaneSource()
plane.SetResolution(res,res)
plane.SetOrigin(0,0,0)
plane.SetPoint1(10,0,0)
plane.SetPoint2(0,10,0)
plane.SetCenter(center)
plane.SetNormal(0,1,0)

# Reuse the locator
locator = vtkStaticPointLocator()
locator.SetDataSet(output)
locator.BuildLocator()

# Voronoi kernel------------------------------------------------
voronoiKernel = vtkVoronoiKernel()

interpolator = vtkPointInterpolator()
interpolator.SetInputConnection(plane.GetOutputPort())
interpolator.SetSourceData(output)
interpolator.SetKernel(voronoiKernel)
interpolator.SetLocator(locator)

# Time execution
timer = vtkTimerLog()
timer.StartTimer()
interpolator.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Interpolate Points (Voronoi): {0}".format(time))

intMapper = vtkPolyDataMapper()
intMapper.SetInputConnection(interpolator.GetOutputPort())

intActor = vtkActor()
intActor.SetMapper(intMapper)

# Create an outline
outline = vtkStructuredGridOutlineFilter()
outline.SetInputData(output)

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)

# Gaussian kernel-------------------------------------------------------
gaussianKernel = vtkGaussianKernel()
#gaussianKernel = vtkEllipsoidalGaussianKernel()
#gaussianKernel.UseScalarsOn()
#gaussianKernel.UseNormalsOn()
gaussianKernel.SetSharpness(4)
gaussianKernel.SetRadius(0.5)

interpolator1 = vtkPointInterpolator()
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

intMapper1 = vtkPolyDataMapper()
intMapper1.SetInputConnection(interpolator1.GetOutputPort())

intActor1 = vtkActor()
intActor1.SetMapper(intMapper1)

# Create an outline
outline1 = vtkStructuredGridOutlineFilter()
outline1.SetInputData(output)

outlineMapper1 = vtkPolyDataMapper()
outlineMapper1.SetInputConnection(outline1.GetOutputPort())

outlineActor1 = vtkActor()
outlineActor1.SetMapper(outlineMapper1)

# Shepard kernel-------------------------------------------------------
shepardKernel = vtkShepardKernel()
shepardKernel.SetPowerParameter(2)
shepardKernel.SetRadius(0.5)

interpolator2 = vtkPointInterpolator()
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

intMapper2 = vtkPolyDataMapper()
intMapper2.SetInputConnection(interpolator2.GetOutputPort())

intActor2 = vtkActor()
intActor2.SetMapper(intMapper2)

# Create an outline
outline2 = vtkStructuredGridOutlineFilter()
outline2.SetInputData(output)

outlineMapper2 = vtkPolyDataMapper()
outlineMapper2.SetInputConnection(outline2.GetOutputPort())

outlineActor2 = vtkActor()
outlineActor2.SetMapper(outlineMapper2)

# Linear kernel-------------------------------------------------------
linearKernel = vtkLinearKernel()
linearKernel.SetRadius(0.5)

interpolator3 = vtkPointInterpolator()
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

intMapper3 = vtkPolyDataMapper()
intMapper3.SetInputConnection(interpolator3.GetOutputPort())

intActor3 = vtkActor()
intActor3.SetMapper(intMapper3)

# Create an outline
outline3 = vtkStructuredGridOutlineFilter()
outline3.SetInputData(output)

outlineMapper3 = vtkPolyDataMapper()
outlineMapper3.SetInputConnection(outline3.GetOutputPort())

outlineActor3 = vtkActor()
outlineActor3.SetMapper(outlineMapper3)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = vtkRenderer()
ren0.SetViewport(0,0,.5,.5)
ren1 = vtkRenderer()
ren1.SetViewport(0.5,0,1,.5)
ren2 = vtkRenderer()
ren2.SetViewport(0,0.5,.5,1)
ren3 = vtkRenderer()
ren3.SetViewport(0.5,0.5,1,1)
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.AddRenderer(ren3)
iren = vtkRenderWindowInteractor()
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
