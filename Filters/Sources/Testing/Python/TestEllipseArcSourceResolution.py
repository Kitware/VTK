#!/usr/bin/env python

import math
from vtkmodules.vtkFiltersSources import vtkEllipseArcSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderer,
)
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Demonstrates the generation of an elliptical arc

arc=vtkEllipseArcSource()
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

m=vtkPolyDataMapper()
m.SetInputData(arc.GetOutput())
a = vtkActor()
a.SetMapper(m)
a.GetProperty().SetColor(1,1,0)
a.GetProperty().EdgeVisibilityOn()
a.GetProperty().RenderLinesAsTubesOn()
a.GetProperty().SetLineWidth(3)
a.GetProperty().SetVertexColor(1,0,0)
a.GetProperty().VertexVisibilityOn()
a.GetProperty().RenderPointsAsSpheresOn()
a.GetProperty().SetPointSize(6)

r=vtkRenderer()
r.AddActor(a)
r.SetBackground(.4,.4,.4)
renWin=vtkRenderWindow()
renWin.AddRenderer(r)
renWin.Render()

# --- end of script ---
