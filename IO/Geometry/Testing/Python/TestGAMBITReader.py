#!/usr/bin/env python
from vtkmodules.vtkIOGeometry import vtkGAMBITReader
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

# Read some Fluent GAMBIT in ASCII form
reader = vtkGAMBITReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/prism.neu")
mapper = vtkDataSetMapper()
mapper.SetInputConnection(reader.GetOutputPort())
actor = vtkActor()
actor.SetMapper(mapper)
# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
renWin.SetSize(300,300)
iren.Initialize()
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
