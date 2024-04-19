#!/usr/bin/env python
from vtkmodules.vtkFiltersCore import vtkContourFilter
from vtkmodules.vtkImagingCore import vtkImageAppendComponents
from vtkmodules.vtkImagingSources import vtkImageGaussianSource
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

# get the interactor ui
## Graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
gs1 = vtkImageGaussianSource()
gs1.SetWholeExtent(0,31,0,31,0,31)
gs1.SetCenter(10,16,16)
gs1.SetMaximum(1000)
gs1.SetStandardDeviation(7)
gs2 = vtkImageGaussianSource()
gs2.SetWholeExtent(0,31,0,31,0,31)
gs2.SetCenter(22,16,16)
gs2.SetMaximum(1000)
gs2.SetStandardDeviation(7)
iac = vtkImageAppendComponents()
iac.AddInputConnection(gs1.GetOutputPort())
iac.AddInputConnection(gs2.GetOutputPort())
cf1 = vtkContourFilter()
cf1.SetInputConnection(iac.GetOutputPort())
cf1.SetValue(0,500)
cf1.SetArrayComponent(0)
cf2 = vtkContourFilter()
cf2.SetInputConnection(iac.GetOutputPort())
cf2.SetValue(0,500)
cf2.SetArrayComponent(1)
mapper1 = vtkPolyDataMapper()
mapper1.SetInputConnection(cf1.GetOutputPort())
mapper1.SetScalarRange(0,1)
mapper1.SetScalarVisibility(0)
mapper1.Update()
mapper2 = vtkPolyDataMapper()
mapper2.SetInputConnection(cf2.GetOutputPort())
mapper2.SetScalarRange(0,1)
mapper2.SetScalarVisibility(0)
actor1 = vtkActor()
actor1.SetMapper(mapper1)
actor1.GetProperty().SetColor(1,1,1)
ren1.AddActor(actor1)
actor2 = vtkActor()
actor2.SetMapper(mapper2)
actor2.GetProperty().SetColor(1,0,0)
ren1.AddActor(actor2)
# Add the actors to the renderer, set the background and size
#
ren1.SetBackground(.3,.3,.3)
renWin.SetSize(400,400)
# enable user interface interactor
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
