#!/usr/bin/env python
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
from vtkmodules.vtkFiltersCore import vtkTriangleFilter
from vtkmodules.vtkFiltersGeometry import vtkDataSetSurfaceFilter
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkImagingCore import vtkExtractVOI, vtkRTAnalyticSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderer,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
)

VTK_DATA_ROOT = vtkGetDataRoot()

# to mark the origin
sphere = vtkSphereSource()
sphere.SetRadius(2.0)

sphereMapper = vtkPolyDataMapper()
sphereMapper.SetInputConnection(sphere.GetOutputPort())

sphereActor = vtkActor()
sphereActor.SetMapper(sphereMapper)

rt = vtkRTAnalyticSource()
rt.SetWholeExtent(-40, 60, -25, 75, 0, 0)
rt.Update()
im = rt.GetOutput()
im.SetDirectionMatrix(-1, 0, 0, 0, -1, 0, 0, 0, 1)

voi = vtkExtractVOI()
voi.SetInputData(im)
voi.SetVOI(-11, 39, 5, 45, 0, 0)
voi.SetSampleRate(5, 5, 1)

# Get rid of ambiguous triangulation issues.
surf = vtkDataSetSurfaceFilter()
surf.SetInputConnection(voi.GetOutputPort())

tris = vtkTriangleFilter()
tris.SetInputConnection(surf.GetOutputPort())

mapper = vtkPolyDataMapper()
mapper.SetInputConnection(tris.GetOutputPort())
mapper.SetScalarRange(130, 280)

actor = vtkActor()
actor.SetMapper(mapper)

ren = vtkRenderer()
ren.AddActor(actor)
ren.AddActor(sphereActor)
ren.ResetCamera()

# camera = ren.GetActiveCamera()
# camera.SetPosition(68.1939, -23.4323, 12.6465)
# camera.SetViewUp(0.46563, 0.882375, 0.0678508)
# camera.SetFocalPoint(3.65707, 11.4552, 1.83509)
# camera.SetClippingRange(59.2626, 101.825)

renWin = vtkRenderWindow()
renWin.AddRenderer(ren)

dm = voi.GetOutput().GetDirectionMatrix()
if dm.GetElement(0, 0) != -1 or dm.GetElement(1, 1) != -1 or dm.GetElement(2, 2) != 1:
    print("ERROR: vtkExtractVOI not passing DirectionMatrix unchanged")

# Verify that mixed sample rates produce the correct output origin.
# With SampleRate > 1 in one or more dims the output extent for those dims
# is reset to start at 0, so the origin must move to the physical location
# of the VOI minimum.  Dims with SampleRate == 1 keep their original extent
# and origin unchanged.
rt2 = vtkRTAnalyticSource()
rt2.SetWholeExtent(-10, 10, -10, 10, -10, 10)
rt2.Update()
im2 = rt2.GetOutput()  # default origin (0,0,0), spacing (1,1,1)


def checkOrigin(label, voi_range, sample_rate, expected):
    v = vtkExtractVOI()
    v.SetInputData(im2)
    v.SetVOI(*voi_range)
    v.SetSampleRate(*sample_rate)
    v.Update()
    got = v.GetOutput().GetOrigin()
    if any(abs(got[i] - expected[i]) > 1e-10 for i in range(3)):
        print(f"ERROR: {label}: expected origin {expected}, got {got}")


# x resampled (extent reset to 0), y and z preserved
checkOrigin(
    "SampleRate (2,1,1)", (-10, 10, -10, 10, 0,
                           0), (2, 1, 1), (-10.0, 0.0, 0.0)
)
# z resampled, x and y preserved
checkOrigin("SampleRate (1,1,2)", (-5, 5, -5, 5, -10, 10),
            (1, 1, 2), (0.0, 0.0, -10.0))
# all dims resampled
checkOrigin(
    "SampleRate (2,2,2)", (-10, 10, -10, 10, -10,
                           10), (2, 2, 2), (-10.0, -10.0, -10.0)
)
# all dims at rate 1 (baseline: origin must not change)
checkOrigin("SampleRate (1,1,1)", (-5, 5, -5, 5, -5, 5),
            (1, 1, 1), (0.0, 0.0, 0.0))

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

iren.Initialize()
# iren.Start()
