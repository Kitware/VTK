#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


import signal
from vtkpython import *

# This script calculates the luminanace of an image

imgWin = vtkImageWindow()


# Image pipeline

image = vtkBMPReader()
image.SetFileName(VTK_DATA + "/beach.bmp")

shrink = vtkImageShrink3D()
shrink.SetInput(image.GetOutput())
shrink.SetShrinkFactors(4,4,1)

operators = ['ByPass','And','Nand','Xor','Or','Nor']

mask = {}
mapper = {}
actor = {}
imager = {}
for operator in operators:
    mask[operator] = vtkImageMaskBits()
    mask[operator].SetInput(shrink.GetOutput())
    if (operator != 'ByPass'):
        getattr(mask[operator],'SetOperationTo'+operator)()
    else:
        mask[operator].BypassOn()
    mask[operator].SetMasks(255,255,0)
    mapper[operator] = vtkImageMapper()
    mapper[operator].SetInput(mask[operator].GetOutput())
    mapper[operator].SetColorWindow(255)
    mapper[operator].SetColorLevel(127.5)
    actor[operator] = vtkActor2D()
    actor[operator].SetMapper(mapper[operator])
    imager[operator] = vtkImager()
    imager[operator].AddActor2D(actor[operator])
    imgWin.AddImager(imager[operator])

column = 1
row = 1
deltaX = 1.0/3.0
deltaY = 1.0/2.0

for operator in operators:
    imager[operator].SetViewport((column-1)*deltaX,(row-1)*deltaY,\
                                 column*deltaX,row*deltaY)
    column = column + 1
    if (column > 3):
        column = 1
        row = row + 1

imgWin.SetSize(384,256)
imgWin.Render()
imgWin.SetFileName("TestAllMaskBits.tcl.ppm")

#imgWin.SaveImageAsPPM()

signal.pause()
