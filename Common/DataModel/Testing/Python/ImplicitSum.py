#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import (
    vtkCone,
    vtkImplicitSum,
    vtkSphere,
)
from vtkmodules.vtkFiltersCore import vtkContourFilter
from vtkmodules.vtkImagingHybrid import vtkSampleFunction
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

# This example demonstrates adding two implicit models
# to produce an (unexpected!) result
# first we load in the standard vtk packages into tcl
geomObject1 = vtkCone()
geomObject2 = vtkSphere()
geomObject2.SetRadius(0.5)
geomObject2.SetCenter(0.5,0,0)
sum = vtkImplicitSum()
sum.SetNormalizeByWeight(1)
sum.AddFunction(geomObject1,2)
sum.AddFunction(geomObject2,1)
sample = vtkSampleFunction()
sample.SetImplicitFunction(sum)
sample.SetSampleDimensions(60,60,60)
sample.ComputeNormalsOn()
surface = vtkContourFilter()
surface.SetInputConnection(sample.GetOutputPort())
surface.SetValue(0,0.0)
mapper = vtkPolyDataMapper()
mapper.SetInputConnection(surface.GetOutputPort())
mapper.ScalarVisibilityOff()
actor = vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetDiffuseColor(0.2,0.4,0.6)
actor.GetProperty().SetSpecular(0.4)
actor.GetProperty().SetDiffuse(0.7)
actor.GetProperty().SetSpecularPower(40)
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.SetBackground(1,1,1)
renWin.SetSize(300,300)
ren1.ResetCamera()
ren1.GetActiveCamera().Azimuth(60)
ren1.GetActiveCamera().Elevation(-10)
ren1.GetActiveCamera().Dolly(1.5)
ren1.ResetCameraClippingRange()
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# render the image
#
renWin.Render()
# --- end of script --
