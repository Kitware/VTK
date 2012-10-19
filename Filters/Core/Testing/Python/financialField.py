#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

size = 3187
#maximum number possible
#set size 100;#maximum number possible
xAxis = "INTEREST_RATE"
yAxis = "MONTHLY_PAYMENT"
zAxis = "MONTHLY_INCOME"
scalar = "TIME_LATE"
# extract data from field as a polydata (just points), then extract scalars
fdr = vtk.vtkDataObjectReader()
fdr.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/financial.vtk")
do2ds = vtk.vtkDataObjectToDataSetFilter()
do2ds.SetInputConnection(fdr.GetOutputPort())
do2ds.SetDataSetTypeToPolyData()
#format: component#, arrayname, arraycomp, minArrayId, maxArrayId, normalize
do2ds.DefaultNormalizeOn()
do2ds.SetPointComponent(0,xAxis,0)
do2ds.SetPointComponent(1,yAxis,0,0,size,1)
do2ds.SetPointComponent(2,zAxis,0)
do2ds.Update()
fd2ad = vtk.vtkFieldDataToAttributeDataFilter()
fd2ad.SetInputConnection(do2ds.GetOutputPort())
fd2ad.SetInputFieldToDataObjectField()
fd2ad.SetOutputAttributeDataToPointData()
fd2ad.DefaultNormalizeOn()
fd2ad.SetScalarComponent(0,scalar,0)
# construct pipeline for original population
popSplatter = vtk.vtkGaussianSplatter()
popSplatter.SetInputConnection(fd2ad.GetOutputPort())
popSplatter.SetSampleDimensions(50,50,50)
popSplatter.SetRadius(0.05)
popSplatter.ScalarWarpingOff()
popSurface = vtk.vtkMarchingContourFilter()
popSurface.SetInputConnection(popSplatter.GetOutputPort())
popSurface.SetValue(0,0.01)
popMapper = vtk.vtkPolyDataMapper()
popMapper.SetInputConnection(popSurface.GetOutputPort())
popMapper.ScalarVisibilityOff()
popActor = vtk.vtkActor()
popActor.SetMapper(popMapper)
popActor.GetProperty().SetOpacity(0.3)
popActor.GetProperty().SetColor(.9,.9,.9)
# construct pipeline for delinquent population
lateSplatter = vtk.vtkGaussianSplatter()
lateSplatter.SetInputConnection(fd2ad.GetOutputPort())
lateSplatter.SetSampleDimensions(50,50,50)
lateSplatter.SetRadius(0.05)
lateSplatter.SetScaleFactor(0.05)
lateSurface = vtk.vtkMarchingContourFilter()
lateSurface.SetInputConnection(lateSplatter.GetOutputPort())
lateSurface.SetValue(0,0.01)
lateMapper = vtk.vtkPolyDataMapper()
lateMapper.SetInputConnection(lateSurface.GetOutputPort())
lateMapper.ScalarVisibilityOff()
lateActor = vtk.vtkActor()
lateActor.SetMapper(lateMapper)
lateActor.GetProperty().SetColor(1.0,0.0,0.0)
# create axes
popSplatter.Update()
bounds = popSplatter.GetOutput().GetBounds()
axes = vtk.vtkAxes()
axes.SetOrigin(lindex(bounds,0),lindex(bounds,2),lindex(bounds,4))
axes.SetScaleFactor(expr.expr(globals(), locals(),["popSplatter.GetOutput().GetLength()","/","5.0"]))
axesTubes = vtk.vtkTubeFilter()
axesTubes.SetInputConnection(axes.GetOutputPort())
axesTubes.SetRadius(expr.expr(globals(), locals(),["axes.GetScaleFactor()","/","25.0"]))
axesTubes.SetNumberOfSides(6)
axesMapper = vtk.vtkPolyDataMapper()
axesMapper.SetInputConnection(axesTubes.GetOutputPort())
axesActor = vtk.vtkActor()
axesActor.SetMapper(axesMapper)
# label the axes
XText = vtk.vtkVectorText()
XText.SetText(xAxis)
XTextMapper = vtk.vtkPolyDataMapper()
XTextMapper.SetInputConnection(XText.GetOutputPort())
XActor = vtk.vtkFollower()
XActor.SetMapper(XTextMapper)
XActor.SetScale(0.02,.02,.02)
XActor.SetPosition(0.35,-0.05,-0.05)
XActor.GetProperty().SetColor(0,0,0)
YText = vtk.vtkVectorText()
YText.SetText(yAxis)
YTextMapper = vtk.vtkPolyDataMapper()
YTextMapper.SetInputConnection(YText.GetOutputPort())
YActor = vtk.vtkFollower()
YActor.SetMapper(YTextMapper)
YActor.SetScale(0.02,.02,.02)
YActor.SetPosition(-0.05,0.35,-0.05)
YActor.GetProperty().SetColor(0,0,0)
ZText = vtk.vtkVectorText()
ZText.SetText(zAxis)
ZTextMapper = vtk.vtkPolyDataMapper()
ZTextMapper.SetInputConnection(ZText.GetOutputPort())
ZActor = vtk.vtkFollower()
ZActor.SetMapper(ZTextMapper)
ZActor.SetScale(0.02,.02,.02)
ZActor.SetPosition(-0.05,-0.05,0.35)
ZActor.GetProperty().SetColor(0,0,0)
# Graphics stuff
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetWindowName("vtk - Field Data")
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(axesActor)
ren1.AddActor(lateActor)
ren1.AddActor(XActor)
ren1.AddActor(YActor)
ren1.AddActor(ZActor)
ren1.AddActor(popActor)
#it's last because its translucent
ren1.SetBackground(1,1,1)
renWin.SetSize(400,400)
camera = vtk.vtkCamera()
camera.SetClippingRange(.274,13.72)
camera.SetFocalPoint(0.433816,0.333131,0.449)
camera.SetPosition(-1.96987,1.15145,1.49053)
camera.SetViewUp(0.378927,0.911821,0.158107)
ren1.SetActiveCamera(camera)
XActor.SetCamera(camera)
YActor.SetCamera(camera)
ZActor.SetCamera(camera)
# render the image
#
iren.Initialize()
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
