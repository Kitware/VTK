#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


import signal
from vtkpython import *

# append multiple displaced spheres into an RGB image.

# Image pipeline

imgWin = vtkImageWindow()

sphere1 = vtkImageEllipsoidSource()
sphere1.SetCenter(95,100,0)
sphere1.SetRadius(70,70,70)

sphere2 = vtkImageEllipsoidSource()
sphere2.SetCenter(161,100,0)
sphere2.SetRadius(70,70,70)

logic = {}
mapper = {}
actor = {}
imager = {}

for operator in ['And','Or','Xor','Nand','Nor','Not']:
   logic[operator] = vtkImageLogic()
   logic[operator].SetInput1(sphere1.GetOutput())
   logic[operator].SetInput2(sphere2.GetOutput())
   logic[operator].SetOutputTrueValue(150)
   getattr(logic[operator],'SetOperationTo'+operator)()
   mapper[operator] = vtkImageMapper()
   mapper[operator].SetInput(logic[operator].GetOutput())
   mapper[operator].SetColorWindow(255)
   mapper[operator].SetColorLevel(127.5)
   actor[operator] = vtkActor2D()
   actor[operator].SetMapper(mapper[operator])
   imager[operator] = vtkImager()
   imager[operator].AddActor2D(actor[operator])
   imgWin.AddImager(imager[operator])
 
imager['And'].SetViewport(0,.5,.33,1)
imager['Or'].SetViewport(.33,.5,.66,1)
imager['Xor'].SetViewport(.66,.5,1,1)
imager['Nand'].SetViewport(0,0,.33,.5)
imager['Nor'].SetViewport(.33,0,.66,.5)
imager['Not'].SetViewport(.66,0,1,.5)

imgWin.SetSize(768,512)
imgWin.Render()
imgWin.SetFileName("TestAllLogic.tcl.ppm")
#imgWin.SaveImageAsPPM()

signal.pause()
