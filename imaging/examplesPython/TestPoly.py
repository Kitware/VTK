#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


import signal
from vtkpython import *

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)

mapper2 = vtkImageMapper()
mapper2.SetInput(reader.GetOutput())
mapper2.SetColorWindow(2000)
mapper2.SetColorLevel(1000)
mapper2.SetZSlice(50)

actor2 = vtkActor2D()
actor2.SetMapper(mapper2)

vtext = vtkVectorText()
vtext.SetText("VTK.Baby!")

trans = vtkTransform()
trans.Scale(25,25,25)

tpd = vtkTransformPolyDataFilter()
tpd.SetTransform(trans)
tpd.SetInput(vtext.GetOutput())

textMapper = vtkPolyDataMapper2D()
textMapper.SetInput(tpd.GetOutput())

coord = vtkCoordinate()
coord.SetCoordinateSystemToNormalizedViewport()
coord.SetValue(0.5,0.5)

textActor = vtkActor2D()
textActor.SetMapper(textMapper)
textActor.GetProperty().SetColor(0.7,0.7,1.0)
textActor.GetPositionCoordinate().SetReferenceCoordinate(coord)
textActor.GetPositionCoordinate().SetCoordinateSystemToViewport()
textActor.GetPositionCoordinate().SetValue(-100,-20)

imager1 = vtkImager()
imager1.AddActor2D(textActor)

imgWin = vtkImageWindow()
imgWin.AddImager(imager1)
imgWin.Render()

signal.pause()
