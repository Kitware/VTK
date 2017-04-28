#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Show the constant kernel.  Smooth an impulse function.
imageCanvas = vtk.vtkImageCanvasSource2D()
imageCanvas.SetScalarTypeToDouble()
imageCanvas.SetExtent(1, 256, 1, 256, 0, 0)
# back ground zero
imageCanvas.SetDrawColor(0)
imageCanvas.FillBox(1, 256, 1, 256)
imageCanvas.SetDrawColor(255)
imageCanvas.FillBox(30, 225, 30, 225)
imageCanvas.SetDrawColor(0)
imageCanvas.FillBox(60, 195, 60, 195)
imageCanvas.SetDrawColor(255)
imageCanvas.FillTube(100, 100, 154, 154, 40.0)
imageCanvas.SetDrawColor(0)
imageCanvas.DrawSegment(45, 45, 45, 210)
imageCanvas.DrawSegment(45, 210, 210, 210)
imageCanvas.DrawSegment(210, 210, 210, 45)
imageCanvas.DrawSegment(210, 45, 45, 45)
imageCanvas.DrawSegment(100, 150, 150, 100)
imageCanvas.DrawSegment(110, 160, 160, 110)
imageCanvas.DrawSegment(90, 140, 140, 90)
imageCanvas.DrawSegment(120, 170, 170, 120)
imageCanvas.DrawSegment(80, 130, 130, 80)
imageCanvas.Update()

shotNoiseAmplitude = 255.0
shotNoiseFraction = 0.1

# set shotNoiseExtent "1 256 1 256 0 0"
shotNoiseSource = vtk.vtkImageNoiseSource()
shotNoiseSource.SetWholeExtent(1, 256, 1, 256, 0, 0)
# $shotNoiseExtent
shotNoiseSource.SetMinimum(0.0)
shotNoiseSource.SetMaximum(1.0)
shotNoiseSource.ReleaseDataFlagOff()

shotNoiseThresh1 = vtk.vtkImageThreshold()
shotNoiseThresh1.SetInputConnection(shotNoiseSource.GetOutputPort())
shotNoiseThresh1.ThresholdByLower(1.0 - shotNoiseFraction)
shotNoiseThresh1.SetInValue(0)
shotNoiseThresh1.SetOutValue(shotNoiseAmplitude)
shotNoiseThresh1.Update()

shotNoiseThresh2 = vtk.vtkImageThreshold()
shotNoiseThresh2.SetInputConnection(shotNoiseSource.GetOutputPort())
shotNoiseThresh2.ThresholdByLower(shotNoiseFraction)
shotNoiseThresh2.SetInValue(-shotNoiseAmplitude)
shotNoiseThresh2.SetOutValue(0.0)
shotNoiseThresh2.Update()

shotNoise = vtk.vtkImageMathematics()
shotNoise.SetInput1Data(shotNoiseThresh1.GetOutput())
shotNoise.SetInput2Data(shotNoiseThresh2.GetOutput())
shotNoise.SetOperationToAdd()
shotNoise.Update()

add = vtk.vtkImageMathematics()
add.SetInput1Data(shotNoise.GetOutput())
add.SetInput2Data(imageCanvas.GetOutput())
add.SetOperationToAdd()

median = vtk.vtkImageMedian3D()
median.SetInputConnection(add.GetOutputPort())
median.SetKernelSize(3, 3, 1)

hybrid1 = vtk.vtkImageHybridMedian2D()
hybrid1.SetInputConnection(add.GetOutputPort())
hybrid2 = vtk.vtkImageHybridMedian2D()
hybrid2.SetInputConnection(hybrid1.GetOutputPort())

viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(hybrid1.GetOutputPort())
viewer.SetColorWindow(256)
viewer.SetColorLevel(127.5)
viewer.GetRenderWindow().SetSize(256, 256)
viewer.Render()
