#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkPoints
from vtkmodules.vtkCommonTransforms import (
    vtkGeneralTransform,
    vtkThinPlateSplineTransform,
)
from vtkmodules.vtkFiltersCore import vtkPolyDataNormals
from vtkmodules.vtkFiltersGeneral import vtkTransformPolyDataFilter
from vtkmodules.vtkFiltersHybrid import (
    vtkGridTransform,
    vtkTransformToGrid,
)
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkDataSetMapper,
    vtkRenderWindow,
    vtkRenderer,
)
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# this example tests the warping of PolyData using thin plate splines
# and with grid transforms using different interpolation modes
# create a rendering window
renWin = vtkRenderWindow()
renWin.SetSize(600,300)
sphere = vtkSphereSource()
sphere.SetThetaResolution(20)
sphere.SetPhiResolution(20)
ap = vtkPolyDataNormals()
ap.SetInputConnection(sphere.GetOutputPort())
#---------------------------
# thin plate spline transform
spoints = vtkPoints()
spoints.SetNumberOfPoints(10)
spoints.SetPoint(0,0.000,0.000,0.500)
spoints.SetPoint(1,0.000,0.000,-0.500)
spoints.SetPoint(2,0.433,0.000,0.250)
spoints.SetPoint(3,0.433,0.000,-0.250)
spoints.SetPoint(4,-0.000,0.433,0.250)
spoints.SetPoint(5,-0.000,0.433,-0.250)
spoints.SetPoint(6,-0.433,-0.000,0.250)
spoints.SetPoint(7,-0.433,-0.000,-0.250)
spoints.SetPoint(8,0.000,-0.433,0.250)
spoints.SetPoint(9,0.000,-0.433,-0.250)
tpoints = vtkPoints()
tpoints.SetNumberOfPoints(10)
tpoints.SetPoint(0,0.000,0.000,0.800)
tpoints.SetPoint(1,0.000,0.000,-0.200)
tpoints.SetPoint(2,0.433,0.000,0.350)
tpoints.SetPoint(3,0.433,0.000,-0.150)
tpoints.SetPoint(4,-0.000,0.233,0.350)
tpoints.SetPoint(5,-0.000,0.433,-0.150)
tpoints.SetPoint(6,-0.433,-0.000,0.350)
tpoints.SetPoint(7,-0.433,-0.000,-0.150)
tpoints.SetPoint(8,0.000,-0.233,0.350)
tpoints.SetPoint(9,0.000,-0.433,-0.150)
thin = vtkThinPlateSplineTransform()
thin.SetSourceLandmarks(spoints)
thin.SetTargetLandmarks(tpoints)
thin.SetBasisToR2LogR()
#  thin Inverse
t1 = vtkGeneralTransform()
t1.SetInput(thin)
f11 = vtkTransformPolyDataFilter()
f11.SetInputConnection(ap.GetOutputPort())
f11.SetTransform(t1)
m11 = vtkDataSetMapper()
m11.SetInputConnection(f11.GetOutputPort())
a11 = vtkActor()
a11.SetMapper(m11)
a11.RotateY(90)
a11.GetProperty().SetColor(1,0,0)
#[a11 GetProperty] SetRepresentationToWireframe
ren11 = vtkRenderer()
ren11.SetViewport(0.0,0.5,0.25,1.0)
ren11.ResetCamera(-0.5,0.5,-0.5,0.5,-1,1)
ren11.AddActor(a11)
renWin.AddRenderer(ren11)
# inverse thin plate spline transform
f12 = vtkTransformPolyDataFilter()
f12.SetInputConnection(ap.GetOutputPort())
f12.SetTransform(t1.GetInverse())
m12 = vtkDataSetMapper()
m12.SetInputConnection(f12.GetOutputPort())
a12 = vtkActor()
a12.SetMapper(m12)
a12.RotateY(90)
a12.GetProperty().SetColor(0.9,0.9,0)
#[a12 GetProperty] SetRepresentationToWireframe
ren12 = vtkRenderer()
ren12.SetViewport(0.0,0.0,0.25,0.5)
ren12.ResetCamera(-0.5,0.5,-0.5,0.5,-1,1)
ren12.AddActor(a12)
renWin.AddRenderer(ren12)
#--------------------------
# grid transform, cubic interpolation
gridTrans = vtkTransformToGrid()
gridTrans.SetInput(t1)
gridTrans.SetGridOrigin(-1.5,-1.5,-1.5)
gridTrans.SetGridExtent(0,60,0,60,0,60)
gridTrans.SetGridSpacing(0.05,0.05,0.05)
t2 = vtkGridTransform()
t2.SetDisplacementGridConnection(gridTrans.GetOutputPort())
t2.SetInterpolationModeToCubic()
f21 = vtkTransformPolyDataFilter()
f21.SetInputConnection(ap.GetOutputPort())
f21.SetTransform(t2)
m21 = vtkDataSetMapper()
m21.SetInputConnection(f21.GetOutputPort())
a21 = vtkActor()
a21.SetMapper(m21)
a21.RotateY(90)
a21.GetProperty().SetColor(1,0,0)
#[a21 GetProperty] SetRepresentationToWireframe
ren21 = vtkRenderer()
ren21.SetViewport(0.25,0.5,0.50,1.0)
ren21.ResetCamera(-0.5,0.5,-0.5,0.5,-1,1)
ren21.AddActor(a21)
renWin.AddRenderer(ren21)
# inverse
f22 = vtkTransformPolyDataFilter()
f22.SetInputConnection(ap.GetOutputPort())
f22.SetTransform(t2.GetInverse())
m22 = vtkDataSetMapper()
m22.SetInputConnection(f22.GetOutputPort())
a22 = vtkActor()
a22.SetMapper(m22)
a22.RotateY(90)
a22.GetProperty().SetColor(0.9,0.9,0)
#[a22 GetProperty] SetRepresentationToWireframe
ren22 = vtkRenderer()
ren22.SetViewport(0.25,0.0,0.50,0.5)
ren22.ResetCamera(-0.5,0.5,-0.5,0.5,-1,1)
ren22.AddActor(a22)
renWin.AddRenderer(ren22)
#--------------------------
# grid transform, linear
t3 = vtkGridTransform()
t3.SetDisplacementGridConnection(gridTrans.GetOutputPort())
t3.SetInterpolationModeToLinear()
f31 = vtkTransformPolyDataFilter()
f31.SetInputConnection(ap.GetOutputPort())
f31.SetTransform(t3)
m31 = vtkDataSetMapper()
m31.SetInputConnection(f31.GetOutputPort())
a31 = vtkActor()
a31.SetMapper(m31)
a31.RotateY(90)
a31.GetProperty().SetColor(1,0,0)
#[a31 GetProperty] SetRepresentationToWireframe
ren31 = vtkRenderer()
ren31.SetViewport(0.50,0.5,0.75,1.0)
ren31.ResetCamera(-0.5,0.5,-0.5,0.5,-1,1)
ren31.AddActor(a31)
renWin.AddRenderer(ren31)
# inverse
f32 = vtkTransformPolyDataFilter()
f32.SetInputConnection(ap.GetOutputPort())
f32.SetTransform(t3.GetInverse())
m32 = vtkDataSetMapper()
m32.SetInputConnection(f32.GetOutputPort())
a32 = vtkActor()
a32.SetMapper(m32)
a32.RotateY(90)
a32.GetProperty().SetColor(0.9,0.9,0)
#[a32 GetProperty] SetRepresentationToWireframe
ren32 = vtkRenderer()
ren32.SetViewport(0.5,0.0,0.75,0.5)
ren32.ResetCamera(-0.5,0.5,-0.5,0.5,-1,1)
ren32.AddActor(a32)
renWin.AddRenderer(ren32)
#--------------------------
# grid transform, nearest
t4 = vtkGridTransform()
t4.SetDisplacementGridConnection(gridTrans.GetOutputPort())
t4.SetInterpolationModeToNearestNeighbor()
t4.SetInverseTolerance(0.05)
f41 = vtkTransformPolyDataFilter()
f41.SetInputConnection(ap.GetOutputPort())
f41.SetTransform(t4)
m41 = vtkDataSetMapper()
m41.SetInputConnection(f41.GetOutputPort())
a41 = vtkActor()
a41.SetMapper(m41)
a41.RotateY(90)
a41.GetProperty().SetColor(1,0,0)
#[a41 GetProperty] SetRepresentationToWireframe
ren41 = vtkRenderer()
ren41.SetViewport(0.75,0.5,1.0,1.0)
ren41.ResetCamera(-0.5,0.5,-0.5,0.5,-1,1)
ren41.AddActor(a41)
renWin.AddRenderer(ren41)
#inverse
f42 = vtkTransformPolyDataFilter()
f42.SetInputConnection(ap.GetOutputPort())
f42.SetTransform(t4.GetInverse())
m42 = vtkDataSetMapper()
m42.SetInputConnection(f42.GetOutputPort())
a42 = vtkActor()
a42.SetMapper(m42)
a42.RotateY(90)
a42.GetProperty().SetColor(0.9,0.9,0)
#[a42 GetProperty] SetRepresentationToWireframe
ren42 = vtkRenderer()
ren42.SetViewport(0.75,0.0,1.0,0.5)
ren42.ResetCamera(-0.5,0.5,-0.5,0.5,-1,1)
ren42.AddActor(a42)
renWin.AddRenderer(ren42)
t1.RotateX(-100)
t1.PostMultiply()
t1.RotateX(+100)
renWin.Render()
# --- end of script --
