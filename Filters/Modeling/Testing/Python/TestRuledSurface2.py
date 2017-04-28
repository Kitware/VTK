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
sphere = vtk.vtkSphereSource()
sphere.SetPhiResolution(15)
sphere.SetThetaResolution(30)
plane = vtk.vtkPlane()
plane.SetNormal(1,0,0)
cut = vtk.vtkCutter()
cut.SetInputConnection(sphere.GetOutputPort())
cut.SetCutFunction(plane)
cut.GenerateCutScalarsOn()
strip = vtk.vtkStripper()
strip.SetInputConnection(cut.GetOutputPort())
points = vtk.vtkPoints()
points.InsertPoint(0,1,0,0)
lines = vtk.vtkCellArray()
lines.InsertNextCell(2)
#number of points
lines.InsertCellPoint(0)
lines.InsertCellPoint(0)
tip = vtk.vtkPolyData()
tip.SetPoints(points)
tip.SetLines(lines)
appendPD = vtk.vtkAppendPolyData()
appendPD.AddInputConnection(strip.GetOutputPort())
appendPD.AddInputData(tip)
# extrude profile to make coverage
#
extrude = vtk.vtkRuledSurfaceFilter()
extrude.SetInputConnection(appendPD.GetOutputPort())
extrude.SetRuledModeToPointWalk()
clean = vtk.vtkCleanPolyData()
clean.SetInputConnection(extrude.GetOutputPort())
clean.ConvertPolysToLinesOff()
mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(clean.GetOutputPort())
mapper.ScalarVisibilityOff()
actor = vtk.vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetOpacity(.4)
ren1.AddActor(actor)
renWin.SetSize(200,200)
renWin.Render()
# render the image
#
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
