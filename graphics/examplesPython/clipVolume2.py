#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

# Example demonstrates how to generate a 3D tetrahedra mesh from a volume. This example
# differs from clipVolume.py in that the mesh is generated within a range of contour
# values.
#

# Quadric definition
quadric = vtkQuadric()
quadric.SetCoefficients(.5,1,.2,0,.1,0,0,.2,0,0)

sample = vtkSampleFunction()
sample.SetSampleDimensions(20,20,20)
sample.SetImplicitFunction(quadric)
sample.ComputeNormalsOff()
    
# Program a bandpass filter to clip a range of data. What we do is transform the 
# scalars so that values lying betweeen (minRange,maxRange) are >= 0.0; all 
# others are < 0.0,
dataset = vtkImplicitDataSet()
dataset.SetDataSet(sample.GetOutput())
window = vtkImplicitWindowFunction()
window.SetImplicitFunction(dataset)
window.SetWindowRange(0.5,1.0)

# Generate tetrahedral mesh
clip = vtkClipVolume()
clip.SetInput(sample.GetOutput())
clip.SetClipFunction(window)
clip.SetValue(0.0)
clip.GenerateClippedOutputOff()

clipMapper = vtkDataSetMapper()
clipMapper.SetInput(clip.GetOutput())
clipMapper.ScalarVisibilityOff()

clipActor = vtkActor()
clipActor.SetMapper(clipMapper)
clipActor.GetProperty().SetColor(.8,.4,.4)

# Create outline
outline = vtkOutlineFilter()
outline.SetInput(clip.GetInput())

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())

outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)

# Define graphics objects
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(400,400)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren.SetBackground(1,1,1)
ren.AddActor(clipActor)
ren.AddActor(outlineActor)

iren.Initialize()
iren.Start()
