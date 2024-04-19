#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import vtkPerlinNoise
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

# A simple example of a three-dimensional noise pattern.
# first we load in the standard vtk packages into tcl
perlin = vtkPerlinNoise()
perlin.SetFrequency(2,1.25,1.5)
perlin.SetPhase(0,0,0)
sample = vtkSampleFunction()
sample.SetImplicitFunction(perlin)
sample.SetSampleDimensions(65,65,20)
sample.ComputeNormalsOff()
surface = vtkContourFilter()
surface.SetInputConnection(sample.GetOutputPort())
surface.SetValue(0,0.0)
mapper = vtkPolyDataMapper()
mapper.SetInputConnection(surface.GetOutputPort())
mapper.ScalarVisibilityOff()
actor = vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetColor(0.2,0.4,0.6)
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
ren1.GetActiveCamera().Dolly(1.35)
ren1.ResetCameraClippingRange()
iren.Initialize()
# render the image
#
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
