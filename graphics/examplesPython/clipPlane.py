#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *

#catch  load vtktcl 

# get the interactor ui
#source ../../examplesTcl/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create a plane 
#
plane = vtkPlaneSource()
plane.SetResolution(5,5)

planeTris = vtkTriangleFilter()
planeTris.SetInput(plane.GetOutput())

halfPlane = vtkPlane()
halfPlane.SetOrigin(-.13,-.03,0)
halfPlane.SetNormal(1,.2,0)

planeClipper = vtkClipPolyData()
planeClipper.GenerateClipScalarsOn()
planeClipper.SetClipFunction(halfPlane)
planeClipper.SetInput(planeTris.GetOutput())

planeMapper = vtkPolyDataMapper()
planeMapper.SetInput(planeClipper.GetOutput())
planeMapper.ScalarVisibilityOff()

planeActor = vtkActor()
planeActor.SetMapper(planeMapper)
planeActor.GetProperty().SetDiffuseColor(0,0,0)
planeActor.GetProperty().SetRepresentationToWireframe()
# Add the actors to the renderer, set the background and size
#
ren.AddActor(planeActor)
ren.SetBackground(1,1,1)
renWin.SetSize(300,300)

#renWin SetFileName "clipPlane.tcl.ppm"
#renWin SaveImageAsPPM


# render the image
#
iren.Initialize()

# prevent the tk window from showing up then start the event loop
#wm withdraw .


iren.Start()
