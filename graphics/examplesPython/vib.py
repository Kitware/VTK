#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *

#catch  load vtktcl 
# this is a tcl version of plate vibration
# get the interactor ui
#source ../../examplesTcl/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# read a vtk file
#
plate = vtkPolyDataReader()
plate.SetFileName("../../../vtkdata/plate.vtk")
plate.SetVectorsName("mode2")
normals = vtkPolyDataNormals()
normals.SetInput(plate.GetOutput())
warp = vtkWarpVector()
warp.SetInput(normals.GetOutput())
warp.SetScaleFactor(0.5)
color = vtkVectorDot()
color.SetInput(warp.GetOutput())
plateMapper = vtkDataSetMapper()
plateMapper.SetInput(warp.GetOutput())
#    plateMapper SetInput [color GetOutput]
plateActor = vtkActor()
plateActor.SetMapper(plateMapper)

# create the outline
#
outline = vtkOutlineFilter()
outline.SetInput(plate.GetOutput())
spikeMapper = vtkPolyDataMapper()
spikeMapper.SetInput(outline.GetOutput())
outlineActor = vtkActor()
outlineActor.SetMapper(spikeMapper)
outlineActor.GetProperty().SetColor(0.0,0.0,0.0)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(plateActor)
ren.AddActor(outlineActor)
ren.SetBackground(0.2,0.3,0.4)
renWin.SetSize(500,500)

# render the image
#
iren.Initialize()
#renWin SetFileName "vib.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
#wm withdraw .


iren.Start()
