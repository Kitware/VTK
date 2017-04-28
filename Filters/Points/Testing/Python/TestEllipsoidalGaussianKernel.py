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

# Create a volume to interpolate on to
volume = vtk.vtkImageData()
volume.SetDimensions(res,res,res)
volume.SetOrigin(0,0,0)
volume.SetSpacing(1,1,1)
fa = vtk.vtkFloatArray()
fa.SetName("scalars")
fa.Allocate(res ** 3)
volume.GetPointData().SetScalars(fa)

center = volume.GetCenter()
bounds = volume.GetBounds()

# Create a single point with a normal and scalar
onePts = vtk.vtkPoints()
onePts.SetNumberOfPoints(1)
onePts.SetPoint(0,center)

oneScalars = vtk.vtkFloatArray()
oneScalars.SetNumberOfTuples(1)
oneScalars.SetTuple1(0,5.0)
oneScalars.SetName("scalarPt")

oneNormals = vtk.vtkFloatArray()
oneNormals.SetNumberOfComponents(3)
oneNormals.SetNumberOfTuples(1)
oneNormals.SetTuple3(0,1,1,1)
oneNormals.SetName("normalPt")

oneData = vtk.vtkPolyData()
oneData.SetPoints(onePts)
oneData.GetPointData().SetScalars(oneScalars)
oneData.GetPointData().SetNormals(oneNormals)

#  Interpolation ------------------------------------------------
eKernel = vtk.vtkEllipsoidalGaussianKernel()
eKernel.SetKernelFootprintToRadius()
eKernel.SetRadius(50.0)
eKernel.UseScalarsOn()
eKernel.UseNormalsOn()
eKernel.SetScaleFactor(0.5)
eKernel.SetEccentricity(3)
eKernel.NormalizeWeightsOff()

interpolator = vtk.vtkPointInterpolator()
interpolator.SetInputData(volume)
interpolator.SetSourceData(oneData)
interpolator.SetKernel(eKernel)
interpolator.Update()

#  Extract iso surface ------------------------------------------------
contour = vtk.vtkFlyingEdges3D()
contour.SetInputConnection(interpolator.GetOutputPort())
contour.SetValue(0,10)

intMapper = vtk.vtkPolyDataMapper()
intMapper.SetInputConnection(contour.GetOutputPort())

intActor = vtk.vtkActor()
intActor.SetMapper(intMapper)

# Create an outline
outline = vtk.vtkOutlineFilter()
outline.SetInputData(volume)

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)

ren0.AddActor(intActor)
#ren0.AddActor(outlineActor)
ren0.SetBackground(1,1,1)

iren.Initialize()
renWin.Render()

iren.Start()
