#!/usr/bin/env python
from vtkmodules.vtkFiltersCore import vtkDataSetToDataObjectFilter
from vtkmodules.vtkIOLegacy import vtkUnstructuredGridReader
from vtkmodules.vtkRenderingCore import (
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
from vtkmodules.vtkRenderingAnnotation import vtkParallelCoordinatesActor
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This example converts data to a field and then displays it using
# parallel coordinates,
# Create a reader and write out the field
reader = vtkUnstructuredGridReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/blow.vtk")
reader.SetVectorsName("displacement9")
reader.SetScalarsName("thickness9")
ds2do = vtkDataSetToDataObjectFilter()
ds2do.SetInputConnection(reader.GetOutputPort())
ds2do.ModernTopologyOff() # Backwards compatibility
ds2do.Update()
actor = vtkParallelCoordinatesActor()
actor.SetInputConnection(ds2do.GetOutputPort())
actor.SetTitle("Parallel Coordinates Plot of blow.tcl")
actor.SetIndependentVariablesToColumns()
actor.GetPositionCoordinate().SetValue(0.05,0.05,0.0)
actor.GetPosition2Coordinate().SetValue(0.95,0.85,0.0)
actor.GetProperty().SetColor(1,0,0)
# Set text colors (same as actor for backward compat with test)
actor.GetTitleTextProperty().SetColor(1,0,0)
actor.GetLabelTextProperty().SetColor(1,0,0)
# Create the RenderWindow, Renderer and both Actors
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren1.AddActor(actor)
ren1.SetBackground(1,1,1)
renWin.SetSize(500,200)
# render the image
#
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
