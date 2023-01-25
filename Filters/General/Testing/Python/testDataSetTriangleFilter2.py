#!/usr/bin/env python
from vtkmodules.vtkFiltersGeneral import (
    vtkDataSetTriangleFilter,
    vtkShrinkFilter,
)
from vtkmodules.vtkIOLegacy import vtkDataSetReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkDataSetMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer, and RenderWindowInteractor
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# create pipeline
#
reader = vtkDataSetReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/uGridEx.vtk")
tris = vtkDataSetTriangleFilter()
tris.SetInputConnection(reader.GetOutputPort())
shrink = vtkShrinkFilter()
shrink.SetInputConnection(tris.GetOutputPort())
shrink.SetShrinkFactor(.8)
mapper = vtkDataSetMapper()
mapper.SetInputConnection(shrink.GetOutputPort())
mapper.SetScalarRange(0,26)
actor = vtkActor()
actor.SetMapper(mapper)
# add the actor to the renderer; set the size
#
ren1.AddActor(actor)
renWin.SetSize(350,350)
ren1.SetBackground(1,1,1)
ren1.GetActiveCamera().SetPosition(-4.01115,6.03964,10.5393)
ren1.GetActiveCamera().SetFocalPoint(1,0.525,3.025)
ren1.GetActiveCamera().SetViewAngle(30)
ren1.GetActiveCamera().SetViewUp(0.114284,0.835731,-0.537115)
ren1.GetActiveCamera().SetClippingRange(4.83787,17.8392)
renWin.Render()
# render the image
#
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
