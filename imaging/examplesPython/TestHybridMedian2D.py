#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# Show the constant kernel.  Smooth an impulse function.


canvas = vtkImageCanvasSource2D()
canvas.SetScalarType(VTK_FLOAT)
canvas.SetExtent(0,255,0,255,0,0)
# back ground zero
canvas.SetDrawColor(0)
canvas.FillBox(0,255,0,255)

canvas.SetDrawColor(255)
canvas.FillBox(30,225,30,225)

canvas.SetDrawColor(0)
canvas.FillBox(60,195,60,195)

canvas.SetDrawColor(255)
canvas.FillTube(100,100,154,154,40.0)

canvas.SetDrawColor(0)

canvas.DrawSegment(45,45,45,210)
canvas.DrawSegment(45,210,210,210)
canvas.DrawSegment(210,210,210,45)
canvas.DrawSegment(210,45,45,45)

canvas.DrawSegment(100,150,150,100)
canvas.DrawSegment(110,160,160,110)
canvas.DrawSegment(90,140,140,90)
canvas.DrawSegment(120,170,170,120)
canvas.DrawSegment(80,130,130,80)




shotNoiseAmplitude = 255.0
shotNoiseFraction  = 0.1
shotNoiseExtent = (0,255,0,255,0,0)

shotNoiseSource = vtkImageNoiseSource()
shotNoiseSource.SetWholeExtent(shotNoiseExtent)
shotNoiseSource.SetMinimum(0.0)
shotNoiseSource.SetMaximum(1.0)
shotNoiseSource.ReleaseDataFlagOff()

shotNoiseThresh1 = vtkImageThreshold()
shotNoiseThresh1.SetInput(shotNoiseSource.GetOutput())
shotNoiseThresh1.ThresholdByLower(1.0-shotNoiseFraction)
shotNoiseThresh1.SetInValue(0)
shotNoiseThresh1.SetOutValue(shotNoiseAmplitude)

shotNoiseThresh2 = vtkImageThreshold()
shotNoiseThresh2.SetInput(shotNoiseSource.GetOutput())
shotNoiseThresh2.ThresholdByLower(shotNoiseFraction)
shotNoiseThresh2.SetInValue(-shotNoiseAmplitude)
shotNoiseThresh2.SetOutValue(0.0)

shotNoise = vtkImageMathematics()
shotNoise.SetInput1(shotNoiseThresh1.GetOutput())
shotNoise.SetInput2(shotNoiseThresh2.GetOutput())
shotNoise.SetOperationToAdd()




add = vtkImageMathematics()
add.SetInput1(shotNoise.GetOutput())
add.SetInput2(canvas.GetOutput())
add.SetOperationToAdd()





median = vtkImageMedian3D()
median.SetInput(add.GetOutput())
median.SetKernelSize(3,3,1)

hybrid1 = vtkImageHybridMedian2D()
hybrid1.SetInput(add.GetOutput())

hybrid2 = vtkImageHybridMedian2D()
hybrid2.SetInput(hybrid1.GetOutput())

viewer = vtkImageViewer()
viewer.SetInput(hybrid1.GetOutput())
viewer.SetColorWindow(256)
viewer.SetColorLevel(127.5)

WindowLevelInterface(viewer)
