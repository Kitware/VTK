#!/usr/bin/env python

# This example demonstrates the use of vtkSTLReader to load data into
# VTK from a file.  This example also uses vtkLODActor which changes
# its graphical representation of the data to maintain interactive
# performance.

import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the reader and read a data file.  Connect the mapper and
# actor.
sr = vtk.vtkSTLReader()
sr.SetFileName(VTK_DATA_ROOT + "/Data/42400-IDGH.stl")

stlMapper = vtk.vtkPolyDataMapper()
stlMapper.SetInput(sr.GetOutput())

stlActor = vtk.vtkLODActor()
stlActor.SetMapper(stlMapper)

# Create the Renderer, RenderWindow, and RenderWindowInteractor
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the render; set the background and size
ren.AddActor(stlActor)
ren.SetBackground(0.1, 0.2, 0.4)
renWin.SetSize(500, 500)

# Zoom in closer
cam1 = ren.GetActiveCamera()
cam1.Zoom(1.4)

iren.Initialize()
renWin.Render()
iren.Start()
