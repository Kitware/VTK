#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *


ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(500,500)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Quadric definition
quadric = vtkQuadric()
quadric.SetCoefficients(.5,1,.2,0,.1,0,0,.2,0,0)

sample = vtkSampleFunction()
sample.SetSampleDimensions(30,30,30)
sample.SetImplicitFunction(quadric)
sample.Update()
#sample.Print()
    
# Create five surfaces F(x,y,z) = constant between range specified
contours = vtkContourFilter()
contours.SetInput(sample.GetOutput())
contours.GenerateValues(5,0.0,1.2)

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

ren.SetBackground(1,1,1)
ren.AddActor(contActor)
ren.AddActor(outlineActor)

iren.Initialize()


iren.Start()
