#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *

#catch  load vtktcl 
# get the interactor ui and colors
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
reader = vtkSTLReader()
reader.SetFileName("../../../vtkdata/42400-IDGH.stl")
dicer = vtkDicer()
dicer.SetInput(reader.GetOutput())
dicer.SetNumberOfPointsPerPiece(1000)
dicer.Update()
isoMapper = vtkDataSetMapper()
isoMapper.SetInput(dicer.GetOutput())
isoMapper.SetScalarRange(0,dicer.GetNumberOfPieces())
isoActor = vtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetColor(raw_sienna[0],raw_sienna[1],raw_sienna[2])

outline = vtkOutlineFilter()
outline.SetInput(reader.GetOutput())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(outlineActor)
ren.AddActor(isoActor)
ren.SetBackground(1,1,1)
renWin.SetSize(500,500)
ren.SetBackground(slate_grey)

# render the image
#
renWin.Render()

#renWin SetFileName "dicer.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
#wm withdraw .

iren.Start()
