#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

from colors import *
# create pipeline
#
v16 = vtkVolume16Reader()
v16.SetDataDimensions(128,128)
v16.GetOutput().SetOrigin(0.0,0.0,0.0)
v16.SetDataByteOrderToLittleEndian()
v16.SetFilePrefix(VTK_DATA + "/headsq/half")
v16.SetImageRange(45,45)
v16.SetDataSpacing(1.6,1.6,1.5)
iso = vtkContourFilter()
iso.SetInput(v16.GetOutput())
iso.GenerateValues(6,600,1200)
cpd = vtkCleanPolyData()
cpd.SetInput(iso.GetOutput())

stripper = vtkStripper()
stripper.SetInput(cpd.GetOutput())
tuber = vtkTubeFilter()
tuber.SetInput(stripper.GetOutput())
tuber.SetNumberOfSides(4)
isoMapper = vtkPolyDataMapper()
isoMapper.SetInput(tuber.GetOutput())
isoMapper.SetScalarRange(600,1200)
isoActor = vtkActor()
isoActor.SetMapper(isoMapper)

outline = vtkOutlineFilter()
outline.SetInput(v16.GetOutput())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp=outlineActor.GetProperty() #eval $outlineProp SetColor 0 0 0

# The graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(outlineActor)
ren.AddActor(isoActor)
ren.SetBackground(1,1,1)
renWin.SetSize(400,400)
ren.GetActiveCamera().Zoom(1.4)
ren.SetBackground(0.1,0.2,0.4)

iren.Initialize()




iren.Start()
