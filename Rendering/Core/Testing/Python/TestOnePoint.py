#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
points = vtk.vtkPoints()
points.InsertPoint(0,0.0,0.0,0.0)
verts = vtk.vtkCellArray()
verts.InsertNextCell(1)
verts.InsertCellPoint(0)
polyData = vtk.vtkPolyData()
polyData.SetPoints(points)
polyData.SetVerts(verts)
mapper = vtk.vtkPolyDataMapper()
mapper.SetInputData(polyData)
actor = vtk.vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetPointSize(8)
ren1.AddViewProp(actor)
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
