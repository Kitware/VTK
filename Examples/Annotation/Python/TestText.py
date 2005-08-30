#!/usr/bin/env python

# This example demonstrates the use of 2D text.

import vtk

# Create a sphere source, mapper, and actor
sphere = vtk.vtkSphereSource()

sphereMapper = vtk.vtkPolyDataMapper()
sphereMapper.SetInputConnection(sphere.GetOutputPort())
sphereMapper.GlobalImmediateModeRenderingOn()
sphereActor = vtk.vtkLODActor()
sphereActor.SetMapper(sphereMapper)

# Create a scaled text actor. 
# Set the text, font, justification, and properties (bold, italics,
# etc.).
textActor = vtk.vtkTextActor()
textActor.ScaledTextOn()
textActor.SetDisplayPosition(90, 50)
textActor.SetInput("This is a sphere")

# Set coordinates to match the old vtkScaledTextActor default value
textActor.GetPosition2Coordinate().SetCoordinateSystemToNormalizedViewport()
textActor.GetPosition2Coordinate().SetValue(0.6, 0.1)

tprop = textActor.GetTextProperty()
tprop.SetFontSize(18)
tprop.SetFontFamilyToArial()
tprop.SetJustificationToCentered()
tprop.BoldOn()
tprop.ItalicOn()
tprop.ShadowOn()
tprop.SetColor(0, 0, 1)

# Create the Renderer, RenderWindow, RenderWindowInteractor
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer; set the background and size; zoom
# in; and render.
ren.AddActor2D(textActor)
ren.AddActor(sphereActor)

ren.SetBackground(1, 1, 1)
renWin.SetSize(250, 125)
ren.ResetCamera()
ren.GetActiveCamera().Zoom(1.5)

iren.Initialize()
renWin.Render()
iren.Start()
