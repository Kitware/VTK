#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

# plate vibration

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
plate.SetFileName(VTK_DATA + "/plate.vtk")
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

iren.Start()
