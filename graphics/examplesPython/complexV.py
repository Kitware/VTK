#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *

#catch  load vtktcl 
# get the interactor ui
#source ../../examplesTcl/vtkInt.tcl
#source ../../examplesTcl/colors.tcl
from colors import *
# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create pipeline
#
reader = vtkStructuredPointsReader()
reader.SetFileName("../../../vtkdata/carotid.vtk")
hhog = vtkHedgeHog()
hhog.SetInput(reader.GetOutput())
hhog.SetScaleFactor(0.3)
lut = vtkLookupTable()
#    lut SetHueRange .667 0.0
lut.Build()
hhogMapper = vtkPolyDataMapper()
hhogMapper.SetInput(hhog.GetOutput())
hhogMapper.SetScalarRange(50,550)
hhogMapper.SetLookupTable(lut)
hhogActor = vtkActor()
hhogActor.SetMapper(hhogMapper)

outline = vtkOutlineFilter()
outline.SetInput(reader.GetOutput())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp=outlineActor.GetProperty() #eval $outlineProp SetColor 0 0 0

# Add the actors to the renderer, set the background and size
#
ren.AddActor(outlineActor)
ren.AddActor(hhogActor)
ren.SetBackground(1,1,1)
renWin.SetSize(500,500)
#renWin SetSize 1000 1000
ren.SetBackground(0.1,0.2,0.4)
iren.Initialize()

# render the image
#
ren.GetActiveCamera().Zoom(1.5)
renWin.Render()
#renWin SetFileName "complexV.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
#wm withdraw .


iren.Start()
