#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *

# This example demonstrates how to use some plotting objects.


# Create a 1D axis
axis = vtkAxisActor2D()
axis.SetNumberOfLabels(5)
axis.SetTitle("X-Axis")
axis.GetPoint1Coordinate().SetCoordinateSystemToNormalizedViewport()
axis.GetPoint1Coordinate().SetValue(0.25,0.25)
axis.GetPoint2Coordinate().SetCoordinateSystemToNormalizedViewport()
axis.GetPoint2Coordinate().SetValue(0.75,0.25)

axis2 = vtkAxisActor2D()
axis2.SetNumberOfLabels(5)
axis2.SetTitle("Y-Axis")
axis2.GetPoint1Coordinate().SetCoordinateSystemToNormalizedViewport()
axis2.GetPoint1Coordinate().SetValue(0.25,0.75)
axis2.GetPoint2Coordinate().SetCoordinateSystemToNormalizedViewport()
axis2.GetPoint2Coordinate().SetValue(0.25,0.25)
axis2.SetRange(1,0)
axis2.SetFontFactor(0.8)

axis3 = vtkAxisActor2D()
axis3.SetNumberOfLabels(5)
axis3.SetTitle("Z-Axis")
axis3.GetPoint1Coordinate().SetCoordinateSystemToNormalizedViewport()
axis3.GetPoint1Coordinate().SetValue(0.3,0.3)
axis3.GetPoint2Coordinate().SetCoordinateSystemToNormalizedViewport()
axis3.GetPoint2Coordinate().SetValue(0.8,0.8)
axis3.SetRange(-2.4,6.7)
axis3.ShadowOff()
axis3.GetProperty().SetColor(0,1,0)

# Create graphics stuff
# Create the RenderWindow, Renderer and both Actors
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren.AddActor2D(axis)
ren.AddActor2D(axis2)
ren.AddActor2D(axis3)
renWin.SetSize(500,500)

# render the image
#
renWin.Render()
iren.Initialize()

#renWin.SetFileName("plot.tcl.ppm")
#renWin.SaveImageAsPPM()


iren.Start()
