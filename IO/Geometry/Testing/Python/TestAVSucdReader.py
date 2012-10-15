#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Read some AVS UCD data in ASCII form
r = vtk.vtkAVSucdReader()
r.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/cellsnd.ascii.inp")
AVSMapper = vtk.vtkDataSetMapper()
AVSMapper.SetInputConnection(r.GetOutputPort())
AVSActor = vtk.vtkActor()
AVSActor.SetMapper(AVSMapper)
# Read some AVS UCD data in binary form
r2 = vtk.vtkAVSucdReader()
r2.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/cellsnd.bin.inp")
AVSMapper2 = vtk.vtkDataSetMapper()
AVSMapper2.SetInputConnection(r2.GetOutputPort())
AVSActor2 = vtk.vtkActor()
AVSActor2.SetMapper(AVSMapper2)
AVSActor2.AddPosition(5,0,0)
# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(AVSActor)
ren1.AddActor(AVSActor2)
renWin.SetSize(300,150)
iren.Initialize()
renWin.Render()
ren1.GetActiveCamera().Zoom(2)
# prevent the tk window from showing up then start the event loop
# --- end of script --
