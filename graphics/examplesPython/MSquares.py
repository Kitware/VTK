#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

## Test the marching squares contouring class

# Quadric definition
quadric = vtkQuadric()
quadric.SetCoefficients(.5,1,.2,0,.1,0,0,.2,0,0)

sample = vtkSampleFunction()
sample.SetSampleDimensions(30,30,30)
sample.SetImplicitFunction(quadric)
sample.ComputeNormalsOff()

extract = vtkExtractVOI()
extract.SetInput(sample.GetOutput())
extract.SetVOI(0,29,0,29,15,15)
extract.SetSampleRate(1,2,3)

contours = vtkContourFilter()
contours.SetInput(extract.GetOutput())
contours.GenerateValues(13,0.0,1.2)

contMapper = vtkPolyDataMapper()
contMapper.SetInput(contours.GetOutput())
contMapper.SetScalarRange(0.0,1.2)

contActor = vtkActor()
contActor.SetMapper(contMapper)

# Create outline
outline = vtkOutlineFilter()
outline.SetInput(sample.GetOutput())

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())

outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)

# create graphics objects
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(500,500)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren.SetBackground(1,1,1)
ren.AddActor(contActor)
ren.AddActor(outlineActor)

ren.GetActiveCamera().Zoom(1.5)
iren.Initialize()

iren.Start()
