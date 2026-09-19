#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkLookupTable
from vtkmodules.vtkFiltersCore import vtkPolyDataNormals
from vtkmodules.vtkFiltersGeneral import vtkWarpScalar
from vtkmodules.vtkFiltersGeometry import vtkImageDataGeometryFilter
from vtkmodules.vtkIOImage import vtkDEMReader
from vtkmodules.vtkRenderingCore import (
    vtkCamera,
    vtkCameraInterpolator,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
from vtkmodules.vtkRenderingLOD import vtkLODActor
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

lut = vtkLookupTable()
lut.SetHueRange(0.6, 0)
lut.SetSaturationRange(1.0, 0)
lut.SetValueRange(0.5, 1.0)

# Read the data: a height field results
demReader = vtkDEMReader()
demReader.SetFileName(VTK_DATA_ROOT + "/Data/SainteHelens.dem")
demReader.Update()

lo = demReader.GetOutput().GetScalarRange()[0]
hi = demReader.GetOutput().GetScalarRange()[1]

surface = vtkImageDataGeometryFilter()
surface.SetInputConnection(demReader.GetOutputPort())

warp = vtkWarpScalar()
warp.SetInputConnection(surface.GetOutputPort())
warp.SetScaleFactor(1)
warp.UseNormalOn()
warp.SetNormal(0, 0, 1)
warp.Update()

normals = vtkPolyDataNormals()
normals.SetInputData(warp.GetPolyDataOutput())
normals.SetFeatureAngle(60)
normals.SplittingOff()

demMapper = vtkPolyDataMapper()
demMapper.SetInputConnection(normals.GetOutputPort())
demMapper.SetScalarRange(lo, hi)
demMapper.SetLookupTable(lut)

demActor = vtkLODActor()
demActor.SetMapper(demMapper)

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(demActor)
ren1.SetBackground(0, 0, 0)

renWin.SetSize(300, 300)

ren1.SetBackground(0.1, 0.2, 0.4)
# render the image
#
renWin.Render()

view1 = vtkCamera()
view1.SetClippingRange(30972.2, 35983.7)
view1.SetFocalPoint(562835, 5.11498e+006, 2294.5)
view1.SetPosition(562835, 5.11498e+006, 35449.9)
view1.SetViewAngle(30)
view1.SetViewUp(0, 1, 0)

view2 = vtkCamera()
view2.SetClippingRange(9013.43, 13470.4)
view2.SetFocalPoint(562835, 5.11498e+006, 2294.5)
view2.SetPosition(562835, 5.11498e+006, 13269.4)
view2.SetViewAngle(30)
view2.SetViewUp(0, 1, 0)

view3 = vtkCamera()
view3.SetClippingRange(4081.2, 13866.4)
view3.SetFocalPoint(562853, 5.11586e+006, 2450.05)
view3.SetPosition(562853, 5.1144e+006, 10726.6)
view3.SetViewAngle(30)
view3.SetViewUp(0, 0.984808, 0.173648)

view4 = vtkCamera()
view4.SetClippingRange(14.0481, 14048.1)
view4.SetFocalPoint(562880, 5.11652e+006, 2733.15)
view4.SetPosition(562974, 5.11462e+006, 6419.98)
view4.SetViewAngle(30)
view4.SetViewUp(0.0047047, 0.888364, 0.459116)

view5 = vtkCamera()
view5.SetClippingRange(14.411, 14411)
view5.SetFocalPoint(562910, 5.11674e+006, 3027.15)
view5.SetPosition(562414, 5.11568e+006, 3419.87)
view5.SetViewAngle(30)
view5.SetViewUp(-0.0301976, 0.359864, 0.932516)

interpolator = vtkCameraInterpolator()
interpolator.SetInterpolationTypeToSpline()
interpolator.AddCamera(0, view1)
interpolator.AddCamera(5, view2)
interpolator.AddCamera(7.5, view3)
interpolator.AddCamera(9.0, view4)
interpolator.AddCamera(11.0, view5)

camera = vtkCamera()
ren1.SetActiveCamera(camera)

def animate():
    numSteps = 500
    min = interpolator.GetMinimumT()
    max = interpolator.GetMaximumT()
    i = 0
    while i <= numSteps:
        t = float(i) * (max - min) / float(numSteps)
        interpolator.InterpolateCamera(t, camera)
        renWin.Render()
        i += 1


interpolator.InterpolateCamera(8.2, camera)

# Exercise the Get/SetTimedCameras round-trip (used by (de)serialization) and
# verify that a camera interpolator reconstructed purely from the flat array
# interpolates identically to the original.
timedCameras = interpolator.GetTimedCameras()
expectedLength = interpolator.GetNumberOfCameras() * 14
if len(timedCameras) != expectedLength:
    raise RuntimeError(
        f"GetTimedCameras() returned {len(timedCameras)} values, expected {expectedLength}")

interpolator2 = vtkCameraInterpolator()
interpolator2.SetInterpolationTypeToSpline()
interpolator2.SetTimedCameras(timedCameras)

if interpolator2.GetNumberOfCameras() != interpolator.GetNumberOfCameras():
    raise RuntimeError("SetTimedCameras() did not restore the expected number of cameras")

testCamera1 = vtkCamera()
testCamera2 = vtkCamera()
tmin = interpolator.GetMinimumT()
tmax = interpolator.GetMaximumT()
numSteps = 10
for i in range(numSteps + 1):
    t = tmin + (tmax - tmin) * i / float(numSteps)
    interpolator.InterpolateCamera(t, testCamera1)
    interpolator2.InterpolateCamera(t, testCamera2)
    for getter in ("GetPosition", "GetFocalPoint", "GetViewUp", "GetClippingRange"):
        v1 = getattr(testCamera1, getter)()
        v2 = getattr(testCamera2, getter)()
        for a, b in zip(v1, v2):
            if abs(a - b) > 1e-6:
                raise RuntimeError(
                    f"interpolator2 diverges at {t=}: {getter} {v1} vs {v2}")
    if abs(testCamera1.GetViewAngle() - testCamera2.GetViewAngle()) > 1e-6:
        raise RuntimeError(
            f"Round-tripped vtkCameraInterpolator view angle mismatch at {t=}")
    if abs(testCamera1.GetParallelScale() - testCamera2.GetParallelScale()) > 1e-6:
        raise RuntimeError(
            f"Round-tripped vtkCameraInterpolator parallel scale mismatch at {t=}")

# animate()

#iren.Start()
