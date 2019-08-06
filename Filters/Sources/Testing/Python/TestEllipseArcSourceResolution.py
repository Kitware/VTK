#!/usr/bin/env python

import math
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Demonstrates the generation of an elliptical arc

arc=vtk.vtkEllipseArcSource()
arc.SetRatio(.25)
arc.SetResolution(40)
arc.SetStartAngle(45)
arc.SetSegmentAngle(360)
arc.CloseOff()
arc.Update()

assert arc.GetOutput().GetNumberOfPoints() == arc.GetResolution()+1

arc.CloseOn()
arc.Update()

assert arc.GetOutput().GetNumberOfPoints() == arc.GetResolution()

m=vtk.vtkPolyDataMapper()
m.SetInputData(arc.GetOutput())
a = vtk.vtkActor()
a.SetMapper(m)
a.GetProperty().SetColor(1,1,0)
a.GetProperty().EdgeVisibilityOn()
a.GetProperty().RenderLinesAsTubesOn()
a.GetProperty().SetLineWidth(3)
a.GetProperty().SetVertexColor(1,0,0)
a.GetProperty().VertexVisibilityOn()
a.GetProperty().RenderPointsAsSpheresOn()
a.GetProperty().SetPointSize(6)

r=vtk.vtkRenderer()
r.AddActor(a)
r.SetBackground(.4,.4,.4)
renWin=vtk.vtkRenderWindow()
renWin.AddRenderer(r)
renWin.Render()

# --- end of script ---
