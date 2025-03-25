#!/usr/bin/env python
from vtkmodules.vtkCommonTransforms import vtkTransform
from vtkmodules.vtkFiltersCore import (
    vtkAppendPolyData,
    vtkProbeFilter,
    vtkStructuredGridOutlineFilter,
    vtkTubeFilter,
)
from vtkmodules.vtkFiltersGeneral import vtkTransformPolyDataFilter
from vtkmodules.vtkFiltersSources import vtkLineSource
from vtkmodules.vtkIOParallel import vtkMultiBlockPLOT3DReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
from vtkmodules.vtkRenderingAnnotation import vtkXYPlotActor
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# create pipeline
#
pl3d = vtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(VTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
output = pl3d.GetOutput().GetBlock(0)
# create three line probes
line = vtkLineSource()
line.SetResolution(30)
transL1 = vtkTransform()
transL1.Translate(3.7,0.0,28.37)
transL1.Scale(5,5,5)
transL1.RotateY(90)
tf = vtkTransformPolyDataFilter()
tf.SetInputConnection(line.GetOutputPort())
tf.SetTransform(transL1)
probe = vtkProbeFilter()
probe.SetInputConnection(tf.GetOutputPort())
probe.SetSourceData(output)
probe.Update()
transL2 = vtkTransform()
transL2.Translate(9.2,0.0,31.20)
transL2.Scale(5,5,5)
transL2.RotateY(90)
tf2 = vtkTransformPolyDataFilter()
tf2.SetInputConnection(line.GetOutputPort())
tf2.SetTransform(transL2)
probe2 = vtkProbeFilter()
probe2.SetInputConnection(tf2.GetOutputPort())
probe2.SetSourceData(output)
probe2.Update()
transL3 = vtkTransform()
transL3.Translate(13.27,0.0,33.40)
transL3.Scale(4.5,4.5,4.5)
transL3.RotateY(90)
tf3 = vtkTransformPolyDataFilter()
tf3.SetInputConnection(line.GetOutputPort())
tf3.SetTransform(transL3)
probe3 = vtkProbeFilter()
probe3.SetInputConnection(tf3.GetOutputPort())
probe3.SetSourceData(output)
probe3.Update()
appendF = vtkAppendPolyData()
appendF.AddInputData(probe.GetPolyDataOutput())
appendF.AddInputData(probe2.GetPolyDataOutput())
appendF.AddInputData(probe3.GetPolyDataOutput())
tuber = vtkTubeFilter()
tuber.SetInputConnection(appendF.GetOutputPort())
tuber.SetRadius(0.1)
lineMapper = vtkPolyDataMapper()
lineMapper.SetInputConnection(tuber.GetOutputPort())
lineActor = vtkActor()
lineActor.SetMapper(lineMapper)
probe.Update()
probe3.Update()
# probe the line and plot it
xyplot = vtkXYPlotActor()
xyplot.AddDataSetInput(probe.GetOutput())
xyplot.AddDataSetInputConnection(probe2.GetOutputPort())
xyplot.AddDataSetInput(probe3.GetOutput())
xyplot.GetPositionCoordinate().SetValue(0.0,0.67,0)
xyplot.GetPosition2Coordinate().SetValue(1.0,0.33,0)
#relative to Position
xyplot.SetXValuesToArcLength()
xyplot.SetNumberOfXLabels(6)
xyplot.SetTitle("Pressure vs. Arc Length (Zoomed View)")
xyplot.SetXTitle("")
xyplot.SetYTitle("P")
xyplot.SetXRange(.1,.35)
xyplot.SetYRange(.2,.4)
xyplot.GetProperty().SetColor(0,0,0)
xyplot.GetProperty().SetLineWidth(2)
# Set text prop color (same color for backward compat with test)
# Assign same object to all text props
tprop = xyplot.GetTitleTextProperty()
tprop.SetColor(xyplot.GetProperty().GetColor())
xyplot.SetAxisTitleTextProperty(tprop)
xyplot.SetAxisLabelTextProperty(tprop)
xyplot.SetLabelFormat("%-#6.2f")
xyplot2 = vtkXYPlotActor()
xyplot2.AddDataSetInput(probe.GetOutput())
xyplot2.AddDataSetInputConnection(probe2.GetOutputPort())
xyplot2.AddDataSetInputConnection(probe3.GetOutputPort())
xyplot2.GetPositionCoordinate().SetValue(0.00,0.33,0)
xyplot2.GetPosition2Coordinate().SetValue(1.0,0.33,0)
#relative to Position
xyplot2.SetXValuesToNormalizedArcLength()
xyplot2.SetNumberOfXLabels(6)
xyplot2.SetTitle("Pressure vs. Normalized Arc Length")
xyplot2.SetXTitle("")
xyplot2.SetYTitle("P")
xyplot2.PlotPointsOn()
xyplot2.PlotLinesOff()
xyplot2.GetProperty().SetColor(1,0,0)
xyplot2.GetProperty().SetPointSize(2)
# Set text prop color (same color for backward compat with test)
# Assign same object to all text props
tprop = xyplot2.GetTitleTextProperty()
tprop.SetColor(xyplot2.GetProperty().GetColor())
xyplot2.SetAxisTitleTextProperty(tprop)
xyplot2.SetAxisLabelTextProperty(tprop)
xyplot2.SetLabelFormat(xyplot.GetLabelFormat())
xyplot3 = vtkXYPlotActor()
xyplot3.AddDataSetInputConnection(probe.GetOutputPort())
xyplot3.AddDataSetInputConnection(probe2.GetOutputPort())
xyplot3.AddDataSetInputConnection(probe3.GetOutputPort())
xyplot3.GetPositionCoordinate().SetValue(0.0,0.0,0)
xyplot3.GetPosition2Coordinate().SetValue(1.0,0.33,0)
#relative to Position
xyplot3.SetXValuesToIndex()
xyplot3.SetNumberOfXLabels(6)
xyplot3.SetTitle("Pressure vs. Point Id")
xyplot3.SetXTitle("Probe Length")
xyplot3.SetYTitle("P")
xyplot3.PlotPointsOn()
xyplot3.GetProperty().SetColor(0,0,1)
xyplot3.GetProperty().SetPointSize(3)
# Set text prop color (same color for backward compat with test)
# Assign same object to all text props
tprop = xyplot3.GetTitleTextProperty()
tprop.SetColor(xyplot3.GetProperty().GetColor())
xyplot3.SetAxisTitleTextProperty(tprop)
xyplot3.SetAxisLabelTextProperty(tprop)
xyplot3.SetLabelFormat(xyplot.GetLabelFormat())
# draw an outline
outline = vtkStructuredGridOutlineFilter()
outline.SetInputData(output)
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)
# Create graphics stuff
#
ren1 = vtkRenderer()
ren2 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren1.SetBackground(0.6784,0.8471,0.9020)
ren1.SetViewport(0,0,.5,1)
ren1.AddActor(outlineActor)
ren1.AddActor(lineActor)
ren2.SetBackground(1,1,1)
ren2.SetViewport(0.5,0.0,1.0,1.0)
ren2.AddViewProp(xyplot)
ren2.AddViewProp(xyplot2)
ren2.AddViewProp(xyplot3)
renWin.SetSize(790,400)
cam1 = ren1.GetActiveCamera()
cam1.SetClippingRange(3.95297,100)
cam1.SetFocalPoint(8.88908,0.595038,29.3342)
cam1.SetPosition(-12.3332,31.7479,41.2387)
cam1.SetViewUp(0.060772,-0.319905,0.945498)
iren.Initialize()
# render the image
#
# prevent the tk window from showing up then start the event loop
# --- end of script --
