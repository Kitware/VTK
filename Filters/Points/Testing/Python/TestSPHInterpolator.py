#!/usr/bin/env python
import vtk
import vtk.test.Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Parameters for testing
res = 250

# Graphics stuff
ren0 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren0)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create pipeline
#
reader = vtk.vtkXMLUnstructuredGridReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/SPH_Points.vtu")
reader.Update()
output = reader.GetOutput()
scalarRange = output.GetPointData().GetArray("Rho").GetRange()

# Something to sample with
center = output.GetCenter()
bounds = output.GetBounds()
length = output.GetLength()

plane = vtk.vtkPlaneSource()
plane.SetResolution(res,res)
plane.SetOrigin(bounds[0],bounds[2],bounds[4])
plane.SetPoint1(bounds[1],bounds[2],bounds[4])
plane.SetPoint2(bounds[0],bounds[3],bounds[4])
plane.SetCenter(center)
plane.SetNormal(0,0,1)
plane.Push(1.15)
plane.Update()

# SPH kernel------------------------------------------------
# Start with somethign weird: process an empty input
emptyPts = vtk.vtkPoints()
emptyData = vtk.vtkPolyData()
emptyData.SetPoints(emptyPts)

sphKernel = vtk.vtkSPHQuinticKernel()
sphKernel.SetSpatialStep(0.1)

interpolator = vtk.vtkSPHInterpolator()
interpolator.SetInputConnection(plane.GetOutputPort())
interpolator.SetSourceData(emptyData)
interpolator.SetKernel(sphKernel)
interpolator.Update()

timer = vtk.vtkTimerLog()

# SPH kernel------------------------------------------------
interpolator.SetInputConnection(plane.GetOutputPort())
interpolator.SetSourceConnection(reader.GetOutputPort())

# Time execution
timer.StartTimer()
interpolator.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Interpolate Points (SPH): {0}".format(time))

intMapper = vtk.vtkPolyDataMapper()
intMapper.SetInputConnection(interpolator.GetOutputPort())
intMapper.SetScalarModeToUsePointFieldData()
intMapper.SelectColorArray("Rho")
intMapper.SetScalarRange(0,720)

intActor = vtk.vtkActor()
intActor.SetMapper(intMapper)

# Create an outline
outline = vtk.vtkOutlineFilter()
outline.SetInputData(output)

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)

ren0.AddActor(intActor)
ren0.AddActor(outlineActor)
ren0.SetBackground(0.1, 0.2, 0.4)

iren.Initialize()
renWin.Render()

iren.Start()
