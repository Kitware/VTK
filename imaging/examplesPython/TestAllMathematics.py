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
sphere1.SetCenter(40,20,0)
sphere1.SetRadius(30,30,0)
sphere1.SetInValue(.75)
sphere1.SetOutValue(.3)
sphere1.SetOutputScalarTypeToFloat()

sphere2 = vtkImageEllipsoidSource()
sphere2.SetCenter(60,30,0)
sphere2.SetRadius(20,20,20)
sphere2.SetInValue(.2)
sphere2.SetOutValue(.5)
sphere2.SetOutputScalarTypeToFloat()

mathematics = ['Add','Subtract','Multiply','Divide','Invert', \
               'Sin','Cos','Exp','Log','AbsoluteValue','Square', \
               'SquareRoot','Min','Max','ATAN','ATAN2','MultiplyByK', \
               'AddConstant']

mathematic = {}
mapper = {}
actor = {}
imager = {}

for operator in mathematics:
    mathematic[operator] = vtkImageMathematics()
    mathematic[operator].SetInput1(sphere1.GetOutput())
    mathematic[operator].SetInput2(sphere2.GetOutput())
    getattr(mathematic[operator],'SetOperationTo'+operator)()
    mathematic[operator].SetConstantK(.21)
    mathematic[operator].SetConstantC(.1)
    mapper[operator] = vtkImageMapper()
    mapper[operator].SetInput(mathematic[operator].GetOutput())
    mapper[operator].SetColorWindow(2.0)
    mapper[operator].SetColorLevel(.75)
    actor[operator] = vtkActor2D()
    actor[operator].SetMapper(mapper[operator])
    imager[operator] = vtkImager()
    imager[operator].AddActor2D(actor[operator])
    imgWin.AddImager(imager[operator])

column = 1
row = 1
deltaX = 1.0/6.0
deltaY = 1.0/3.0

for operator in mathematics:
    imager[operator].SetViewport((column-1)*deltaX,(row-1)*deltaY, \
                                 column*deltaX,row*deltaY)
    column = column + 1
    if (column > 6):
        column = 1
        row = row + 1

imgWin.SetSize(600,300)
imgWin.Render()
imgWin.SetFileName("TestAllMathematics.tcl.ppm")
#imgWin.SaveImageAsPPM()

signal.pause()
