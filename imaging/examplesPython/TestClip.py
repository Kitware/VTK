#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# Test the object vtkImagePaint which is a region that has methods to draw
# lines and Boxs in different colors.  This extension allows the
# paint object to draw grey scale.


canvas = vtkImageCanvasSource2D()
canvas.SetNumberOfScalarComponents(3)
canvas.SetScalarType(VTK_UNSIGNED_CHAR)
canvas.SetExtent(0,511,0,511,0,0)
canvas.SetDrawColor(100,100,0)
canvas.FillBox(0,511,0,511)
canvas.SetDrawColor(200,0,200)
canvas.FillBox(32,511,100,500)
canvas.SetDrawColor(100,0,0)
canvas.FillTube(550,20,30,400,5)
canvas.SetDrawColor(255,255,255)
canvas.DrawSegment(10,20,90,510)
canvas.SetDrawColor(200,50,50)
canvas.DrawSegment(510,90,10,20)

# Check segment clipping
canvas.SetDrawColor(0,200,0)
canvas.DrawSegment(-10,30,30,-10)
canvas.DrawSegment(-10,481,30,521)
canvas.DrawSegment(481,-10,521,30)
canvas.DrawSegment(481,521,521,481)

# Check Filling a triangle
canvas.SetDrawColor(20,200,200)
canvas.FillTriangle(-100,100,190,150,40,300)

# Check drawing a circle
canvas.SetDrawColor(250,250,10)
canvas.DrawCircle(350,350,200.0)

# Check drawing a point
canvas.SetDrawColor(250,250,250)
canvas.DrawPoint(350,350)
canvas.DrawPoint(350,550)


# Test filling functionality
canvas.SetDrawColor(55,0,0)
canvas.DrawCircle(450,350,80.0)
canvas.SetDrawColor(100,255,100)
canvas.FillPixel(450,350)

clip = vtkImageClip()
clip.SetInput(canvas.GetOutput())
clip.SetOutputWholeExtent(0,255,0,255,0,0)
clip.ReleaseDataFlagOff()

viewer = vtkImageViewer()
viewer.SetInput(clip.GetOutput())
viewer.SetColorWindow(256)
viewer.SetColorLevel(127.5)


# make interface
WindowLevelInterface(viewer)
