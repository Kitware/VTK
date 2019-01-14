#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Parameters for debugging
res = 200
math = vtk.vtkMath()

# create pipeline: use terrain dataset. Just for kicks we'll treat the elevations
# as a "point cloud" and interpolate the elevation onto a plane.
#
# Read the data: a height field results
demReader = vtk.vtkDEMReader()
demReader.SetFileName(VTK_DATA_ROOT + "/Data/SainteHelens.dem")
demReader.Update()

lo = demReader.GetOutput().GetScalarRange()[0]
hi = demReader.GetOutput().GetScalarRange()[1]

bds = demReader.GetOutput().GetBounds()
center = demReader.GetOutput().GetCenter()

# Create a plane of onto which to map the elevation data
plane = vtk.vtkPlaneSource()
plane.SetResolution(res,res)
plane.SetOrigin(bds[0],bds[2],bds[4])
plane.SetPoint1(bds[1],bds[2],bds[4])
plane.SetPoint2(bds[0],bds[3],bds[4])
plane.Update()


# Gaussian kernel-------------------------------------------------------
gaussianKernel = vtk.vtkGaussianKernel()
gaussianKernel.SetSharpness(2)
gaussianKernel.SetRadius(200)

interp = vtk.vtkPointInterpolator2D()
interp.SetInputConnection(plane.GetOutputPort())
interp.SetSourceConnection(demReader.GetOutputPort())
interp.SetKernel(gaussianKernel)
interp.SetNullPointsStrategyToClosestPoint()
interp.GetLocator().SetNumberOfPointsPerBucket(1)
interp.InterpolateZOff()

# Time execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
interp.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Interpolate Terrain Points (Gaussian): {0}".format(time))

scalarRange = interp.GetOutput().GetPointData().GetArray("Elevation").GetRange()

lut = vtk.vtkLookupTable()
lut.SetHueRange(0.6, 0)
lut.SetSaturationRange(1.0, 0)
lut.SetValueRange(0.5, 1.0)

intMapper = vtk.vtkPolyDataMapper()
intMapper.SetInputConnection(interp.GetOutputPort())
intMapper.SetScalarModeToUsePointFieldData()
intMapper.SelectColorArray("Elevation")
intMapper.SetScalarRange(scalarRange)
intMapper.SetLookupTable(lut)

intActor = vtk.vtkActor()
intActor.SetMapper(intMapper)

# Create some contours
cf = vtk.vtkContourFilter()
cf.SetInputConnection(interp.GetOutputPort())
cf.GenerateValues(20,scalarRange)

cfMapper = vtk.vtkPolyDataMapper()
cfMapper.SetInputConnection(cf.GetOutputPort())

cfActor = vtk.vtkActor()
cfActor.SetMapper(cfMapper)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren0)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(intActor)
ren0.AddActor(cfActor)
ren0.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(300, 300)

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
