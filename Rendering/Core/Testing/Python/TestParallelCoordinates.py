#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This example converts data to a field and then displays it using 
# parallel coordinates,
# Create a reader and write out the field
reader = vtk.vtkUnstructuredGridReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/blow.vtk")
reader.SetVectorsName("displacement9")
reader.SetScalarsName("thickness9")
ds2do = vtk.vtkDataSetToDataObjectFilter()
ds2do.SetInputConnection(reader.GetOutputPort())
ds2do.Update()
actor = vtk.vtkParallelCoordinatesActor()
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
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren1.AddActor(actor)
ren1.SetBackground(1,1,1)
renWin.SetSize(500,200)
# render the image
#
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
