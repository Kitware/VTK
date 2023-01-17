#!/usr/bin/env python
from vtkmodules.vtkFiltersExtraction import vtkExtractGrid
from vtkmodules.vtkFiltersGeneral import vtkWarpVector
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

# create pipeline
#
reader = vtkDataSetReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/RectGrid2.vtk")
reader.Update()

warper = vtkWarpVector()
warper.SetInputConnection(reader.GetOutputPort())
warper.SetScaleFactor(0.2)

extract = vtkExtractGrid()
extract.SetInputConnection(warper.GetOutputPort())
extract.SetVOI(0, 100, 0, 100, 7, 15)

mapper = vtkDataSetMapper()
mapper.SetInputConnection(extract.GetOutputPort())
mapper.SetScalarRange(0.197813, 0.710419)

actor = vtkActor()
actor.SetMapper(mapper)

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
ren1.AddActor(actor)
ren1.SetBackground(1, 1, 1)

renWin.SetSize(400, 400)

cam1 = ren1.GetActiveCamera()
cam1.SetClippingRange(3.76213, 10.712)
cam1.SetFocalPoint(-0.0842503, -0.136905, 0.610234)
cam1.SetPosition(2.53813, 2.2678, -5.22172)
cam1.SetViewUp(-0.241047, 0.930635, 0.275343)

# render the image
#
iren.Initialize()
#iren.Start()
