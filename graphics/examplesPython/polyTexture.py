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
renWin.SetSize(400,400)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Plane source to generate texture
plane = vtkPlaneSource()
plane.SetResolution(63,63) #.resolution.specifies.number.of.quads

# Transform for texture and quad
aTransform = vtkTransform()
aTransform.RotateX(30)

planeTransform = vtkTransformPolyDataFilter()
planeTransform.SetTransform(aTransform)
planeTransform.SetInput(plane.GetOutput())

# Generate a synthetic volume from quadric
quadric = vtkQuadric()
quadric.SetCoefficients(.5,1,.2,0,.1,0,0,.2,0,0)

transformSamples = vtkTransform()
transformSamples.RotateX(30)
transformSamples.Inverse()

sample = vtkSampleFunction()
sample.SetSampleDimensions(30,30,30)
sample.SetImplicitFunction(quadric)
sample.Update()
    
# Probe the synthetic volume
probe = vtkProbeFilter()
probe.SetInput(planeTransform.GetOutput())
probe.SetSource(sample.GetOutput())
probe.Update()

# Create Structured points and set the scalars
structuredPoints = vtkStructuredPoints()
structuredPoints.GetPointData().SetScalars(probe.GetOutput().GetPointData().GetScalars())
structuredPoints.SetDimensions(64,64,1) #.these.dimensions.must.match.probe.point.count

# Define the texture with structured points
polyTexture = vtkTexture()
polyTexture.SetInput(structuredPoints)

# The quad we'll see
quad = vtkPlaneSource()
quad.SetResolution(1,1)

# Use the same transform as the probed points
quadTransform = vtkTransformPolyDataFilter()
quadTransform.SetTransform(aTransform)
quadTransform.SetInput(quad.GetOutput())

quadMapper = vtkPolyDataMapper()
quadMapper.SetInput(quadTransform.GetOutput())

quadActor = vtkActor()
quadActor.SetMapper(quadMapper)
quadActor.SetTexture(polyTexture)

# Create outline
outline = vtkOutlineFilter()
outline.SetInput(sample.GetOutput())

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())

outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)

ren.SetBackground(1,1,1)
ren.AddActor(quadActor)
ren.AddActor(outlineActor)
ren.GetActiveCamera().Dolly(1.3)
ren.ResetCameraClippingRange()

iren.Initialize()


# Update the scalars in the structured points with the probe's output
def updateStructuredPoints():
	structuredPoints.GetPointData().SetScalars(probe.GetOutput().GetPointData().GetScalars())
 

# Transform the probe and resample
def resample():
	#.Transform.the.probe.points.and.the.quad()
	aTransform.RotateY(10)
	#.Force.an.update.on.the.probe.since.the.pipeline.is.broken()
	probe.Update()
	renWin.Render()
 

# Set the probes end method to update the scalars in the structured points
probe.SetEndMethod(updateStructuredPoints)

#
for i in range(1,36+1):
	resample()
 
iren.Start()
