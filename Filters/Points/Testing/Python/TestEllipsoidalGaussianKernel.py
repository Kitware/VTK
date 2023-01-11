#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkFloatArray,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import (
    vtkImageData,
    vtkPolyData,
)
from vtkmodules.vtkFiltersCore import vtkFlyingEdges3D
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkFiltersPoints import (
    vtkEllipsoidalGaussianKernel,
    vtkPointInterpolator,
)
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
import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Parameters for testing
res = 250

# Graphics stuff
ren0 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren0)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create a volume to interpolate on to
volume = vtkImageData()
volume.SetDimensions(res,res,res)
volume.SetOrigin(0,0,0)
volume.SetSpacing(1,1,1)
fa = vtkFloatArray()
fa.SetName("scalars")
fa.Allocate(res ** 3)
volume.GetPointData().SetScalars(fa)

center = volume.GetCenter()
bounds = volume.GetBounds()

# Create a single point with a normal and scalar
onePts = vtkPoints()
onePts.SetNumberOfPoints(1)
onePts.SetPoint(0,center)

oneScalars = vtkFloatArray()
oneScalars.SetNumberOfTuples(1)
oneScalars.SetTuple1(0,5.0)
oneScalars.SetName("scalarPt")

oneNormals = vtkFloatArray()
oneNormals.SetNumberOfComponents(3)
oneNormals.SetNumberOfTuples(1)
oneNormals.SetTuple3(0,1,1,1)
oneNormals.SetName("normalPt")

oneData = vtkPolyData()
oneData.SetPoints(onePts)
oneData.GetPointData().SetScalars(oneScalars)
oneData.GetPointData().SetNormals(oneNormals)

#  Interpolation ------------------------------------------------
eKernel = vtkEllipsoidalGaussianKernel()
eKernel.SetKernelFootprintToRadius()
eKernel.SetRadius(50.0)
eKernel.UseScalarsOn()
eKernel.UseNormalsOn()
eKernel.SetScaleFactor(0.5)
eKernel.SetEccentricity(3)
eKernel.NormalizeWeightsOff()

interpolator = vtkPointInterpolator()
interpolator.SetInputData(volume)
interpolator.SetSourceData(oneData)
interpolator.SetKernel(eKernel)
interpolator.Update()

#  Extract iso surface ------------------------------------------------
contour = vtkFlyingEdges3D()
contour.SetInputConnection(interpolator.GetOutputPort())
contour.SetValue(0,10)

intMapper = vtkPolyDataMapper()
intMapper.SetInputConnection(contour.GetOutputPort())

intActor = vtkActor()
intActor.SetMapper(intMapper)

# Create an outline
outline = vtkOutlineFilter()
outline.SetInputData(volume)

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)

ren0.AddActor(intActor)
#ren0.AddActor(outlineActor)
ren0.SetBackground(1,1,1)

iren.Initialize()
renWin.Render()

iren.Start()
