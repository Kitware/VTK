#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Pipeline
reader = vtk.vtkPNGReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/fullhead15.png")
reader.Update()
scalarRange = reader.GetOutput().GetPointData().GetScalars().GetRange()

iso = vtk.vtkFlyingEdges2D()
iso.SetInputConnection(reader.GetOutputPort())
iso.GenerateValues(12,scalarRange)

isoMapper = vtk.vtkPolyDataMapper()
isoMapper.SetInputConnection(iso.GetOutputPort())
isoMapper.SetScalarRange(scalarRange)

isoActor = vtk.vtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetColor(1,1,1)

outline = vtk.vtkOutlineFilter()
outline.SetInputConnection(reader.GetOutputPort())

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(outlineActor)
ren1.AddActor(isoActor)
ren1.SetBackground(0,0,0)
renWin.SetSize(300,300)
ren1.ResetCamera()
iren.Initialize()

renWin.Render()
iren.Start()
# --- end of script --
