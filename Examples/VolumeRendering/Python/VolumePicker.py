#!/usr/bin/env python

##
# This is an example of how to use the vtkVolumePicker.
## 

import math
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

#---------------------------------------------------------
# renderer and interactor
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

#---------------------------------------------------------
# read the volume
reader = vtk.vtkImageReader2()
reader.SetDataExtent(0,63,0,63,0,92)
reader.SetFileNameSliceOffset(1)
reader.SetDataScalarTypeToUnsignedShort()
reader.SetDataByteOrderToLittleEndian()
reader.SetFilePrefix(str(VTK_DATA_ROOT) + "/Data/headsq/quarter")
reader.SetDataSpacing(3.2,3.2,1.5)

#---------------------------------------------------------
# set up the volume rendering
volumeMapper = vtk.vtkVolumeTextureMapper3D()
volumeMapper.SetInput(reader.GetOutput())
volumeMapper.CroppingOn()
volumeMapper.SetCroppingRegionPlanes((0.0, 141.6, 0.0, 201.6, 0.0, 138.0))

volumeColor = vtk.vtkColorTransferFunction()
volumeColor.AddRGBPoint(0,0.0,0.0,0.0)
volumeColor.AddRGBPoint(180,0.3,0.1,0.2)
volumeColor.AddRGBPoint(1000,1.0,0.7,0.6)
volumeColor.AddRGBPoint(2000,1.0,1.0,0.9)

volumeScalarOpacity = vtk.vtkPiecewiseFunction()
volumeScalarOpacity.AddPoint(0,0.0)
volumeScalarOpacity.AddPoint(180,0.0)
volumeScalarOpacity.AddPoint(1000,0.2)
volumeScalarOpacity.AddPoint(2000,0.8)

volumeGradientOpacity = vtk.vtkPiecewiseFunction()
volumeGradientOpacity.AddPoint(0,0.0)
volumeGradientOpacity.AddPoint(90,0.5)
volumeGradientOpacity.AddPoint(100,1.0)

volumeProperty = vtk.vtkVolumeProperty()
volumeProperty.SetColor(volumeColor)
volumeProperty.SetScalarOpacity(volumeScalarOpacity)
volumeProperty.SetGradientOpacity(volumeGradientOpacity)
volumeProperty.SetInterpolationTypeToLinear()
volumeProperty.ShadeOff()
volumeProperty.SetAmbient(0.6)
volumeProperty.SetDiffuse(0.6)
volumeProperty.SetSpecular(0.1)

volume = vtk.vtkVolume()
volume.SetMapper(volumeMapper)
volume.SetProperty(volumeProperty)

#---------------------------------------------------------
# Do the surface rendering
boneExtractor = vtk.vtkMarchingCubes()
boneExtractor.SetInputConnection(reader.GetOutputPort())
boneExtractor.SetValue(0,1150)

boneNormals = vtk.vtkPolyDataNormals()
boneNormals.SetInputConnection(boneExtractor.GetOutputPort())
boneNormals.SetFeatureAngle(60.0)

boneStripper = vtk.vtkStripper()
boneStripper.SetInputConnection(boneNormals.GetOutputPort())

boneLocator = vtk.vtkCellLocator()
boneLocator.SetDataSet(boneExtractor.GetOutput())
boneLocator.LazyEvaluationOn()

boneMapper = vtk.vtkPolyDataMapper()
boneMapper.SetInputConnection(boneStripper.GetOutputPort())
boneMapper.ScalarVisibilityOff()

boneProperty = vtk.vtkProperty()
boneProperty.SetColor(1.0,1.0,0.9)

bone = vtk.vtkActor()
bone.SetMapper(boneMapper)
bone.SetProperty(boneProperty)

#---------------------------------------------------------
# Create an image actor
table = vtk.vtkLookupTable()
table.SetRange(0,2000)
table.SetRampToLinear()
table.SetValueRange(0,1)
table.SetHueRange(0,0)
table.SetSaturationRange(0,0)

mapToColors = vtk.vtkImageMapToColors()
mapToColors.SetInputConnection(reader.GetOutputPort())
mapToColors.SetLookupTable(table)
mapToColors.GetOutput().Update()

imageActor = vtk.vtkImageActor()
imageActor.SetInput(mapToColors.GetOutput())
imageActor.SetDisplayExtent(32,32,0,63,0,92)

#---------------------------------------------------------
# make a transform and some clipping planes
transform = vtk.vtkTransform()
transform.RotateWXYZ(-20,0.0,-0.7,0.7)

volume.SetUserTransform(transform)
bone.SetUserTransform(transform)
imageActor.SetUserTransform(transform)

c = volume.GetCenter()

volumeClip = vtk.vtkPlane()
volumeClip.SetNormal(0,1,0)
volumeClip.SetOrigin(c[0],c[1],c[2])

boneClip = vtk.vtkPlane()
boneClip.SetNormal(1,0,0)
boneClip.SetOrigin(c[0],c[1],c[2])

volumeMapper.AddClippingPlane(volumeClip)
boneMapper.AddClippingPlane(boneClip)

#---------------------------------------------------------
ren.AddViewProp(volume)
ren.AddViewProp(bone)
ren.AddViewProp(imageActor)

camera = ren.GetActiveCamera()
camera.SetFocalPoint(c[0],c[1],c[2])
camera.SetPosition(c[0] + 500,c[1] - 100,c[2] - 100)
camera.SetViewUp(0,0,-1)

renWin.Render()

#---------------------------------------------------------
# the cone points along the -x axis
coneSource = vtk.vtkConeSource()
coneSource.CappingOn()
coneSource.SetHeight(12)
coneSource.SetRadius(5)
coneSource.SetResolution(31)
coneSource.SetCenter(6,0,0)
coneSource.SetDirection(-1,0,0)

coneMapper = vtk.vtkDataSetMapper()
coneMapper.SetInputConnection(coneSource.GetOutputPort())

redCone = vtk.vtkActor()
redCone.PickableOff()
redCone.SetMapper(coneMapper)
redCone.GetProperty().SetColor(1,0,0)

greenCone = vtk.vtkActor()
greenCone.PickableOff()
greenCone.SetMapper(coneMapper)
greenCone.GetProperty().SetColor(0,1,0)

# Add the two cones (or just one, if you want)
ren.AddViewProp(redCone)
ren.AddViewProp(greenCone)

#---------------------------------------------------------
# the picker
picker = vtk.vtkVolumePicker()
picker.SetTolerance(1e-6)
picker.SetVolumeOpacityIsovalue(0.1)
# locator is optional, but improves performance for large polydata
picker.AddLocator(boneLocator)

# A function to point an actor along a vector
def PointCone(actor,nx,ny,nz):
    actor.SetOrientation(0.0, 0.0, 0.0)
    n = math.sqrt(nx**2 + ny**2 + nz**2)
    if (nx < 0.0):
        actor.RotateWXYZ(180, 0, 1, 0)
        n = -n
    actor.RotateWXYZ(180, (nx+n)*0.5, ny*0.5, nz*0.5)

# A function to move the cursor with the mouse
def MoveCursor(iren,event=""):
    renWin.HideCursor()
    x,y = iren.GetEventPosition()
    picker.Pick(x, y, 0, ren)
    p = picker.GetPickPosition()
    n = picker.GetPickNormal()
    redCone.SetPosition(p[0],p[1],p[2])
    PointCone(redCone,n[0],n[1],n[2])
    greenCone.SetPosition(p[0],p[1],p[2])
    PointCone(greenCone,-n[0],-n[1],-n[2])
    iren.Render()

#---------------------------------------------------------
# custom interaction
iren.AddObserver("MouseMoveEvent", MoveCursor)

iren.Start()
