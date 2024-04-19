#!/usr/bin/env python
from vtkmodules.vtkFiltersParallel import vtkRectilinearGridOutlineFilter
from vtkmodules.vtkIOXML import vtkXMLRectilinearGridReader
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

# create pipeline
#
reader = vtkXMLRectilinearGridReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/RectGrid2.vtr")

outline = vtkRectilinearGridOutlineFilter()
outline.SetInputConnection(reader.GetOutputPort())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineMapper.SetNumberOfPieces(2)
outlineMapper.SetPiece(1)
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0.0, 0.0, 0.0)
# Graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(outlineActor)
ren1.SetBackground(1,1,1)
renWin.SetSize(400,400)
cam1 = ren1.GetActiveCamera()
cam1.SetClippingRange(3.76213,10.712)
cam1.SetFocalPoint(-0.0842503,-0.136905,0.610234)
cam1.SetPosition(2.53813,2.2678,-5.22172)
cam1.SetViewUp(-0.241047,0.930635,0.275343)
iren.Initialize()
# render the image
#
# prevent the tk window from showing up then start the event loop
# --- end of script --
