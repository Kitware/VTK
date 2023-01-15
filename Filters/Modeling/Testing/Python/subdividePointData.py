#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkLookupTable
from vtkmodules.vtkFiltersCore import vtkElevationFilter
from vtkmodules.vtkFiltersModeling import (
    vtkButterflySubdivisionFilter,
    vtkLinearSubdivisionFilter,
)
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCamera,
    vtkLight,
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

#
# Test butterfly subdivision of point data
#
sphere = vtkSphereSource()
sphere.SetPhiResolution(11)
sphere.SetThetaResolution(11)
colorIt = vtkElevationFilter()
colorIt.SetInputConnection(sphere.GetOutputPort())
colorIt.SetLowPoint(0,0,-.5)
colorIt.SetHighPoint(0,0,.5)
butterfly = vtkButterflySubdivisionFilter()
butterfly.SetInputConnection(colorIt.GetOutputPort())
butterfly.SetNumberOfSubdivisions(3)
lut = vtkLookupTable()
lut.SetNumberOfColors(256)
lut.Build()
mapper = vtkPolyDataMapper()
mapper.SetInputConnection(butterfly.GetOutputPort())
mapper.SetLookupTable(lut)
actor = vtkActor()
actor.SetMapper(mapper)
linear = vtkLinearSubdivisionFilter()
linear.SetInputConnection(colorIt.GetOutputPort())
linear.SetNumberOfSubdivisions(3)
mapper2 = vtkPolyDataMapper()
mapper2.SetInputConnection(linear.GetOutputPort())
mapper2.SetLookupTable(lut)
actor2 = vtkActor()
actor2.SetMapper(mapper2)
mapper3 = vtkPolyDataMapper()
mapper3.SetInputConnection(colorIt.GetOutputPort())
mapper3.SetLookupTable(lut)
actor3 = vtkActor()
actor3.SetMapper(mapper3)
ren1 = vtkRenderer()
ren2 = vtkRenderer()
ren3 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.AddRenderer(ren3)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren1.AddActor(actor)
ren1.SetBackground(1,1,1)
ren2.AddActor(actor2)
ren2.SetBackground(1,1,1)
ren3.AddActor(actor3)
ren3.SetBackground(1,1,1)
renWin.SetSize(600,200)
aCamera = vtkCamera()
aCamera.Azimuth(70)
aLight = vtkLight()
aLight.SetPosition(aCamera.GetPosition())
aLight.SetFocalPoint(aCamera.GetFocalPoint())
ren1.SetActiveCamera(aCamera)
ren1.AddLight(aLight)
ren1.ResetCamera()
aCamera.Dolly(1.4)
ren1.ResetCameraClippingRange()
ren2.SetActiveCamera(aCamera)
ren2.AddLight(aLight)
ren3.SetActiveCamera(aCamera)
ren3.AddLight(aLight)
ren3.SetViewport(0,0,.33,1)
ren2.SetViewport(.33,0,.67,1)
ren1.SetViewport(.67,0,1,1)
iren.Initialize()
# --- end of script --
