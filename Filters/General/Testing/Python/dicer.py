#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# create pipeline
#
reader = vtk.vtkSTLReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/42400-IDGH.stl")
dicer = vtk.vtkOBBDicer()
dicer.SetInputConnection(reader.GetOutputPort())
dicer.SetNumberOfPointsPerPiece(1000)
dicer.Update()
isoMapper = vtk.vtkDataSetMapper()
isoMapper.SetInputConnection(dicer.GetOutputPort())
isoMapper.SetScalarRange(0,dicer.GetNumberOfActualPieces())
isoActor = vtk.vtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetColor(0.7,0.3,0.3)
outline = vtk.vtkOutlineCornerFilter()
outline.SetInputConnection(reader.GetOutputPort())
outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(outlineActor)
ren1.AddActor(isoActor)
ren1.SetBackground(1,1,1)
renWin.SetSize(400,400)
ren1.SetBackground(0.5,0.5,0.6)
# render the image
#
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
