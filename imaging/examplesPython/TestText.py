#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *

# demonstrate the use of 2D text

sphere = vtkSphereSource()

sphereMapper = vtkPolyDataMapper()
sphereMapper.SetInput(sphere.GetOutput())
sphereMapper.GlobalImmediateModeRenderingOn()
sphereActor = vtkLODActor()
sphereActor.SetMapper(sphereMapper)

textMapper = vtkTextMapper()
textMapper.SetInput("This.is.a.sphere")
textMapper.SetFontSize(18)
textMapper.SetFontFamilyToArial()
textMapper.SetJustificationToCentered()
textMapper.BoldOn()
textMapper.ItalicOn()
textMapper.ShadowOn()
textActor = vtkScaledTextActor()
textActor.SetMapper(textMapper)
textActor.GetProperty().SetColor(0,0,1)

# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren.AddActor2D(textActor)
ren.AddActor(sphereActor)

ren.SetBackground(1,1,1)
renWin.SetSize(250,125)
ren.GetActiveCamera().Zoom(1.5)
renWin.Render()

#renWin.SetFileName("TestText.tcl.ppm")
#renWin.SaveImageAsPPM()

# render the image
#


iren.Start()
