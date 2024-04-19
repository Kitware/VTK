#!/usr/bin/env python

from vtkmodules.vtkCommonCore import vtkLookupTable
from vtkmodules.vtkCommonDataModel import (
    vtkCellLocator,
    vtkPlane,
)
from vtkmodules.vtkFiltersCore import (
    vtkMarchingCubes,
    vtkPolyDataNormals,
    vtkStripper,
)
from vtkmodules.vtkFiltersSources import vtkConeSource
from vtkmodules.vtkIOImage import vtkVolume16Reader
from vtkmodules.vtkImagingCore import vtkImageMapToColors
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCellPicker,
    vtkDataSetMapper,
    vtkImageActor,
    vtkPolyDataMapper,
    vtkProperty,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()


#
# Do picking with a locator to speed things up
#

# renderer and interactor
ren = vtkRenderer()

renWin = vtkRenderWindow()
renWin.AddRenderer(ren)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# read the volume
v16 = vtkVolume16Reader()
v16.SetDataDimensions(64, 64)
v16.SetImageRange(1, 93)
v16.SetDataByteOrderToLittleEndian()
v16.SetFilePrefix(VTK_DATA_ROOT + '/Data/headsq/quarter')
v16.SetDataSpacing(3.2, 3.2, 1.5)

#---------------------------------------------------------
# Do the surface rendering
boneExtractor = vtkMarchingCubes()
boneExtractor.SetInputConnection(v16.GetOutputPort())
boneExtractor.SetValue(0, 1150)

boneNormals = vtkPolyDataNormals()
boneNormals.SetInputConnection(boneExtractor.GetOutputPort())
boneNormals.SetFeatureAngle(60.0)

boneStripper = vtkStripper()
boneStripper.SetInputConnection(boneNormals.GetOutputPort())
boneStripper.SetMaximumLength(5)

boneLocator = vtkCellLocator()
boneLocator.SetDataSet(boneStripper.GetOutput())

boneMapper = vtkPolyDataMapper()
boneMapper.SetInputConnection(boneStripper.GetOutputPort())
boneMapper.ScalarVisibilityOff()

boneProperty = vtkProperty()
boneProperty.SetColor(1.0, 1.0, 0.9)

bone = vtkActor()
bone.SetMapper(boneMapper)
bone.SetProperty(boneProperty)

#---------------------------------------------------------
# Create an image actor

table = vtkLookupTable()
table.SetRange(0, 2000)
table.SetRampToLinear()
table.SetValueRange(0, 1)
table.SetHueRange(0, 0)
table.SetSaturationRange(0, 0)

mapToColors = vtkImageMapToColors()
mapToColors.SetInputConnection(v16.GetOutputPort())
mapToColors.SetLookupTable(table)

imageActor = vtkImageActor()
imageActor.GetMapper().SetInputConnection(mapToColors.GetOutputPort())
imageActor.SetDisplayExtent(32, 32, 0, 63, 0, 92)

#---------------------------------------------------------
# make a clipping plane

cx = 100.8
cy = 100.8
cz = 69.0

# cuts the bone data in half
boneClip = vtkPlane()
boneClip.SetNormal(0, 1, 0)
boneClip.SetOrigin(cx, cy, cz)

# doesn't cut the data, but is within camera near/far planes
boneClip2 = vtkPlane()
boneClip2.SetNormal(-1, 0, 0)
boneClip2.SetOrigin(cx + 100, cy, cz)

boneMapper.AddClippingPlane(boneClip)
boneMapper.AddClippingPlane(boneClip2)

#---------------------------------------------------------
ren.AddViewProp(bone)
ren.AddViewProp(imageActor)

camera = ren.GetActiveCamera()
camera.SetFocalPoint(cx, cy, cz)
camera.SetPosition(cx + 400, cy + 100, cz - 100)
camera.SetViewUp(0, 0, -1)

ren.ResetCameraClippingRange()

renWin.Render()

#---------------------------------------------------------
# the cone should point along the Z axis
coneSource = vtkConeSource()
coneSource.CappingOn()
coneSource.SetHeight(12)
coneSource.SetRadius(5)
coneSource.SetResolution(31)
coneSource.SetCenter(6, 0, 0)
coneSource.SetDirection(-1, 0, 0)

#---------------------------------------------------------
picker = vtkCellPicker()
picker.SetTolerance(1e-6)
picker.AddLocator(boneLocator)

# A function to point an actor along a vector n = (nx,ny.nz)
def PointCone(actor, n):
    if n[0] < 0.0:
        actor.RotateWXYZ(180, 0, 1, 0)
        actor.RotateWXYZ(180, (n[0] - 1.0) * 0.5, n[1] * 0.5, n[2] * 0.5)
    else:
        actor.RotateWXYZ(180, (n[0] + 1.0) * 0.5, n[1] * 0.5, n[2] * 0.5)

# Pick the actor
picker.Pick(70, 120, 0, ren)
#print picker
p = picker.GetPickPosition()
n = picker.GetPickNormal()

coneActor1 = vtkActor()
coneActor1.PickableOff()
coneMapper1 = vtkDataSetMapper()
coneMapper1.SetInputConnection(coneSource.GetOutputPort())
coneActor1.SetMapper(coneMapper1)
coneActor1.GetProperty().SetColor(1, 0, 0)
coneActor1.SetPosition(p)
PointCone(coneActor1, n)
ren.AddViewProp(coneActor1)

# Pick the image
picker.Pick(170, 220, 0, ren)
#print picker
p = picker.GetPickPosition()
n = picker.GetPickNormal()

coneActor2 = vtkActor()
coneActor2.PickableOff()
coneMapper2 = vtkDataSetMapper()
coneMapper2.SetInputConnection(coneSource.GetOutputPort())
coneActor2.SetMapper(coneMapper2)
coneActor2.GetProperty().SetColor(1, 0, 0)
coneActor2.SetPosition(p)
PointCone(coneActor2, n)
ren.AddViewProp(coneActor2)

# Pick the actor again, in a way that the ray gets clipped
picker.Pick(180, 220, 0, ren)
#print picker
p = picker.GetPickPosition()
n = picker.GetPickNormal()

coneActor3 = vtkActor()
coneActor3.PickableOff()
coneMapper3 = vtkDataSetMapper()
coneMapper3.SetInputConnection(coneSource.GetOutputPort())
coneActor3.SetMapper(coneMapper3)
coneActor3.GetProperty().SetColor(1, 0, 0)
coneActor3.SetPosition(p)
PointCone(coneActor3, n)
ren.AddViewProp(coneActor3)

ren.ResetCameraClippingRange()

renWin.Render()

#---------------------------------------------------------
# test-related code
def TkCheckAbort(obj=None, event=""):
    if renWin.GetEventPending():
        renWin.SetAbortRender(1)

renWin.AddObserver("AbortCheckEvent", TkCheckAbort)
