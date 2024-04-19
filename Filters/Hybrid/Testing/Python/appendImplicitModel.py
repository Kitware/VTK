#!/usr/bin/env python
from vtkmodules.vtkFiltersCore import vtkContourFilter
from vtkmodules.vtkFiltersGeometry import vtkImageDataGeometryFilter
from vtkmodules.vtkFiltersHybrid import vtkImplicitModeller
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkFiltersSources import (
    vtkLineSource,
    vtkPlaneSource,
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
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# this demonstrates appending data to generate an implicit model
lineX = vtkLineSource()
lineX.SetPoint1(-2.0,0.0,0.0)
lineX.SetPoint2(2.0,0.0,0.0)
lineX.Update()
lineY = vtkLineSource()
lineY.SetPoint1(0.0,-2.0,0.0)
lineY.SetPoint2(0.0,2.0,0.0)
lineY.Update()
lineZ = vtkLineSource()
lineZ.SetPoint1(0.0,0.0,-2.0)
lineZ.SetPoint2(0.0,0.0,2.0)
lineZ.Update()
aPlane = vtkPlaneSource()
aPlane.Update()
# set Data(0) "lineX"
# set Data(1) "lineY"
# set Data(2) "lineZ"
# set Data(3) "aPlane"
imp = vtkImplicitModeller()
imp.SetModelBounds(-2.5,2.5,-2.5,2.5,-2.5,2.5)
imp.SetSampleDimensions(60,60,60)
imp.SetCapValue(1000)
imp.SetProcessModeToPerVoxel()
# Okay now let's see if we can append
imp.StartAppend()
# for {set i 0} {$i < 4} {incr i} {
#     imp Append [$Data($i) GetOutput]
# }
imp.Append(lineX.GetOutput())
imp.Append(lineY.GetOutput())
imp.Append(lineZ.GetOutput())
imp.Append(aPlane.GetOutput())
imp.EndAppend()
cf = vtkContourFilter()
cf.SetInputConnection(imp.GetOutputPort())
cf.SetValue(0,0.1)
mapper = vtkPolyDataMapper()
mapper.SetInputConnection(cf.GetOutputPort())
actor = vtkActor()
actor.SetMapper(mapper)
outline = vtkOutlineFilter()
outline.SetInputConnection(imp.GetOutputPort())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
plane = vtkImageDataGeometryFilter()
plane.SetInputConnection(imp.GetOutputPort())
plane.SetExtent(0,60,0,60,30,30)
planeMapper = vtkPolyDataMapper()
planeMapper.SetInputConnection(plane.GetOutputPort())
planeMapper.SetScalarRange(0.197813,0.710419)
planeActor = vtkActor()
planeActor.SetMapper(planeMapper)
# graphics stuff
ren1 = vtkRenderer()
ren1.AddActor(actor)
ren1.AddActor(planeActor)
ren1.AddActor(outlineActor)
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren1.SetBackground(0.1,0.2,0.4)
renWin.Render()
ren1.GetActiveCamera().Azimuth(30)
ren1.GetActiveCamera().Elevation(30)
ren1.ResetCameraClippingRange()
renWin.Render()
# --- end of script --
