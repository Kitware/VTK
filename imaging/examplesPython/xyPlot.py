#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


import signal
from vtkpython import *

# Demonstrate the use x-y plots

# create pipeline
#
pl3d = vtkPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA + "/combxyz.bin")
pl3d.SetQFileName(VTK_DATA + "/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()

# create three line probes
line = vtkLineSource()
line.SetResolution(50)

transL1 = vtkTransform()
transL1.Translate(3.7,0.0,28.37)
transL1.Scale(5,5,5)
transL1.RotateY(90)
tf = vtkTransformPolyDataFilter()
tf.SetInput(line.GetOutput())
tf.SetTransform(transL1)
probe = vtkProbeFilter()
probe.SetInput(tf.GetOutput())
probe.SetSource(pl3d.GetOutput())

transL2 = vtkTransform()
transL2.Translate(9.2,0.0,31.20)
transL2.Scale(5,5,5)
transL2.RotateY(90)
tf2 = vtkTransformPolyDataFilter()
tf2.SetInput(line.GetOutput())
tf2.SetTransform(transL2)
probe2 = vtkProbeFilter()
probe2.SetInput(tf2.GetOutput())
probe2.SetSource(pl3d.GetOutput())

transL3 = vtkTransform()
transL3.Translate(13.27,0.0,33.40)
transL3.Scale(4.5,4.5,4.5)
transL3.RotateY(90)
tf3 = vtkTransformPolyDataFilter()
tf3.SetInput(line.GetOutput())
tf3.SetTransform(transL3)
probe3 = vtkProbeFilter()
probe3.SetInput(tf3.GetOutput())
probe3.SetSource(pl3d.GetOutput())

appendF = vtkAppendPolyData()
appendF.AddInput(probe.GetPolyDataOutput())
appendF.AddInput(probe2.GetPolyDataOutput())
appendF.AddInput(probe3.GetPolyDataOutput())
tuber = vtkTubeFilter()
tuber.SetInput(appendF.GetOutput())
tuber.SetRadius(0.1)
lineMapper = vtkPolyDataMapper()
lineMapper.SetInput(tuber.GetOutput())
lineActor = vtkActor()
lineActor.SetMapper(lineMapper)

# probe the line and plot it
xyplot = vtkXYPlotActor()
xyplot.AddInput(probe.GetOutput())
xyplot.AddInput(probe2.GetOutput())
xyplot.AddInput(probe3.GetOutput())
xyplot.GetPositionCoordinate().SetValue(0.0,0.67,0)
xyplot.GetPosition2Coordinate().SetValue(1.0,0.33,0) #relative to Position
xyplot.SetXValuesToArcLength()
xyplot.SetNumberOfXLabels(6)
xyplot.SetTitle("Pressure.vs..Arc.Length.(Zoomed.View)")
xyplot.SetXTitle("")
xyplot.SetYTitle("P")
xyplot.SetXRange(.1,.35)
xyplot.SetYRange(.2,.4)
xyplot.GetProperty().SetColor(0,0,0)

xyplot2 = vtkXYPlotActor()
xyplot2.AddInput(probe.GetOutput())
xyplot2.AddInput(probe2.GetOutput())
xyplot2.AddInput(probe3.GetOutput())
xyplot2.GetPositionCoordinate().SetValue(0.00,0.33,0)
xyplot2.GetPosition2Coordinate().SetValue(1.0,0.33,0) #relative to Position
xyplot2.SetXValuesToNormalizedArcLength()
xyplot2.SetNumberOfXLabels(6)
xyplot2.SetTitle("Pressure.vs..Normalized.Arc.Length")
xyplot2.SetXTitle("")
xyplot2.SetYTitle("P")
xyplot2.GetProperty().SetColor(1,0,0)

xyplot3 = vtkXYPlotActor()
xyplot3.AddInput(probe.GetOutput())
xyplot3.AddInput(probe2.GetOutput())
xyplot3.AddInput(probe3.GetOutput())
xyplot3.GetPositionCoordinate().SetValue(0.0,0.0,0)
xyplot3.GetPosition2Coordinate().SetValue(1.0,0.33,0) #relative to Position
xyplot3.SetXValuesToIndex()
xyplot3.SetNumberOfXLabels(6)
xyplot3.SetTitle("Pressure.vs..Point.Id")
xyplot3.SetXTitle("Probe.Length")
xyplot3.SetYTitle("P")
xyplot3.GetProperty().SetColor(0,0,1)

# draw an outline
outline = vtkStructuredGridOutlineFilter()
outline.SetInput(pl3d.GetOutput())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)

# Create graphics stuff
#
ren = vtkRenderer()
ren2 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.AddRenderer(ren2)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren.SetBackground(0.6784,0.8471,0.9020)
ren.SetViewport(0,0,.5,1)
ren.AddActor(outlineActor)
ren.AddActor(lineActor)

ren2.SetBackground(1,1,1)
ren2.SetViewport(0.5,0.0,1.0,1.0)
ren2.AddActor2D(xyplot)
ren2.AddActor2D(xyplot2)
ren2.AddActor2D(xyplot3)
renWin.SetSize(500,250)

cam1 = ren.GetActiveCamera()
cam1.SetClippingRange(3.95297,100)
cam1.SetFocalPoint(8.88908,0.595038,29.3342)
cam1.SetPosition(-12.3332,31.7479,41.2387)
cam1.SetViewUp(0.060772,-0.319905,0.945498)
iren.Initialize()

renWin.SetFileName("xyPlot.tcl.ppm")
#renWin.SaveImageAsPPM()

# render the image
#
renWin.Render()

signal.pause()
