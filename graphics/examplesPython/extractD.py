#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

# extract data
from colors import *
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

quadric = vtkQuadric()
quadric.SetCoefficients(.5,1,.2,0,.1,0,0,.2,0,0)
sample = vtkSampleFunction()
sample.SetSampleDimensions(50,50,50)
sample.SetImplicitFunction(quadric)
sample.ComputeNormalsOff()
trans = vtkTransform()
trans.Scale(1,.5,.333)
sphere = vtkSphere()
sphere.SetRadius(0.25)
sphere.SetTransform(trans)
trans2 = vtkTransform()
trans2.Scale(.25,.5,1.0)
sphere2 = vtkSphere()
sphere2.SetRadius(0.25)
sphere2.SetTransform(trans2)
union = vtkImplicitBoolean()
union.AddFunction(sphere)
union.AddFunction(sphere2)
union.SetOperationType(0) #union
extract = vtkExtractGeometry()
extract.SetInput(sample.GetOutput())
extract.SetImplicitFunction(union)
shrink = vtkShrinkFilter()
shrink.SetInput(extract.GetOutput())
shrink.SetShrinkFactor(0.5)
dataMapper = vtkDataSetMapper()
dataMapper.SetInput(shrink.GetOutput())
dataActor = vtkActor()
dataActor.SetMapper(dataMapper)

# outline
outline = vtkOutlineFilter()
outline.SetInput(sample.GetOutput())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp=outlineActor.GetProperty()
outlineProp.SetColor(0,0,0)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(outlineActor)
ren.AddActor(dataActor)
ren.SetBackground(1,1,1)
renWin.SetSize(500,500)
ren.GetActiveCamera().Zoom(1.5)
iren.Initialize()

# render the image
#


iren.Start()
