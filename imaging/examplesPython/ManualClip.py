#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# Simple viewer for images.

# Image pipeline

reader = vtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)
#reader.DebugOn()
#reader.Update()

table = vtkLookupTable()

table.SetTableRange(200,3000)
table.SetSaturationRange(0,0)
table.SetHueRange(0,0)
table.SetValueRange(0,1)
table.SetAlphaRange(0,1)
table.Build()

rgba = vtkImageMapToRGBA()
rgba.SetInput(reader.GetOutput())
rgba.SetLookupTable(table)

clip = vtkImageClip()
clip.SetInput(rgba.GetOutput())
clip.SetOutputWholeExtent(0,255,127,127,1,64)
clip.ReleaseDataFlagOff()

viewer = vtkImageViewer()
viewer.SetSize(256,256)
viewer.SetInput(clip.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
#viewer.DebugOn()

viewer.Render()

plane = vtkPlaneSource()

mapper = vtkDataSetMapper()
mapper.SetInput(plane.GetOutput())


texture = vtkTexture()
texture.SetInput(clip.GetOutput())

actor = vtkActor()
actor.SetMapper(mapper)
actor.SetTexture(texture)

ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren.AddActor(actor)
ren.SetBackground(1,1,1)

renWin.SetSize(500,500)
renWin.Render()

iren.Start()

#make interface
WindowLevelInterface(viewer)
