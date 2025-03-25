#!/usr/bin/env python
from vtkmodules.vtkCommonTransforms import vtkTransform
from vtkmodules.vtkFiltersCore import (
    vtkAppendPolyData,
    vtkDataSetToDataObjectFilter,
    vtkProbeFilter,
    vtkStructuredGridOutlineFilter,
    vtkTubeFilter,
)
from vtkmodules.vtkFiltersGeneral import vtkTransformPolyDataFilter
from vtkmodules.vtkFiltersSources import (
    vtkGlyphSource2D,
    vtkLineSource,
)
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
# probe the line and plot it
triangle = vtkGlyphSource2D()
triangle.SetGlyphTypeToTriangle()
triangle.Update()
cross = vtkGlyphSource2D()
cross.SetGlyphTypeToCross()
cross.Update()
xyplot = vtkXYPlotActor()
xyplot.AddDataSetInputConnection(probe.GetOutputPort())
xyplot.AddDataSetInputConnection(probe2.GetOutputPort())
xyplot.AddDataSetInputConnection(probe3.GetOutputPort())
xyplot.GetPositionCoordinate().SetValue(0.0,0.5,0)
xyplot.GetPosition2Coordinate().SetValue(1.0,0.5,0)
#relative to Position
xyplot.SetXValuesToValue()
xyplot.SetPointComponent(0,2)
xyplot.SetPointComponent(1,2)
xyplot.SetPointComponent(2,2)
xyplot.LogxOn()
xyplot.SetNumberOfXLabels(6)
xyplot.SetTitle("Pressure vs. Log10 Probe Z-Value")
xyplot.SetXTitle("")
xyplot.SetYTitle("P")
xyplot.PlotCurveLinesOn()
xyplot.PlotCurvePointsOn()
xyplot.SetPlotLines(0,1)
xyplot.SetPlotLines(1,0)
xyplot.SetPlotLines(2,1)
xyplot.SetPlotPoints(0,0)
xyplot.SetPlotPoints(1,1)
xyplot.SetPlotPoints(2,1)
xyplot.GetProperty().SetColor(0,0,0)
xyplot.GetProperty().SetLineWidth(1)
xyplot.GetProperty().SetPointSize(3)
xyplot.SetPlotSymbol(2,triangle.GetOutput())
xyplot.SetPlotColor(2,0,0,1)
xyplot.SetGlyphSize(0.025)
# Set text prop color (same color for backward compat with test)
# Assign same object to all text props
tprop = xyplot.GetTitleTextProperty()
tprop.SetColor(xyplot.GetProperty().GetColor())
xyplot.SetAxisTitleTextProperty(tprop)
xyplot.SetAxisLabelTextProperty(tprop)
xyplot.SetLabelFormat("%-#6.2f")
#Okay exercise data object stuff
ds2do = vtkDataSetToDataObjectFilter()
ds2do.SetInputConnection(probe.GetOutputPort())
ds2do.ModernTopologyOff() # Backwards compatibility
ds2do.Update()
ds2do2 = vtkDataSetToDataObjectFilter()
ds2do2.SetInputConnection(probe.GetOutputPort())
ds2do2.ModernTopologyOff() # Backwards compatibility
ds2do3 = vtkDataSetToDataObjectFilter()
ds2do3.SetInputConnection(probe.GetOutputPort())
ds2do3.ModernTopologyOff() # Backwards compatibility
ds2do3.Update()
xyplot3 = vtkXYPlotActor()
xyplot3.AddDataObjectInput(ds2do.GetOutput())
xyplot3.SetDataObjectXComponent(0,2)
xyplot3.SetDataObjectYComponent(0,5)
xyplot3.SetPlotColor(0,1,0,0)
xyplot3.SetPlotLabel(0,"Mx")
xyplot3.AddDataObjectInputConnection(ds2do2.GetOutputPort())
xyplot3.SetDataObjectXComponent(1,2)
xyplot3.SetDataObjectYComponent(1,6)
xyplot3.SetPlotColor(1,0,1,0)
xyplot3.SetPlotLabel(1,"My")
xyplot3.AddDataObjectInput(ds2do3.GetOutput())
xyplot3.SetDataObjectXComponent(2,2)
xyplot3.SetDataObjectYComponent(2,7)
xyplot3.SetPlotColor(2,0,0,1)
xyplot3.SetPlotLabel(2,"Mz")
xyplot3.GetPositionCoordinate().SetValue(0.0,0.0,0)
xyplot3.GetPosition2Coordinate().SetValue(1.0,0.5,0)
#relative to Position
xyplot3.SetXValuesToValue()
xyplot3.SetNumberOfXLabels(6)
xyplot3.SetTitle("Momentum Component vs. Log10 Probe Z-Value")
xyplot3.SetXTitle("Log10 Probe Z-Value")
xyplot3.SetYTitle("M")
xyplot3.GetProperty().SetColor(0,0,1)
xyplot3.GetProperty().SetPointSize(5)
xyplot3.PlotCurveLinesOn()
xyplot3.PlotCurvePointsOn()
xyplot3.SetPlotLines(0,1)
xyplot3.SetPlotLines(1,0)
xyplot3.SetPlotLines(2,1)
xyplot3.SetPlotPoints(0,0)
xyplot3.SetPlotPoints(1,1)
xyplot3.SetPlotPoints(2,1)
xyplot3.LogxOn()
# Set text prop color (same color for backward compat with test)
# Assign same object to all text props
tprop = xyplot3.GetTitleTextProperty()
tprop.SetColor(xyplot3.GetProperty().GetColor())
xyplot3.SetAxisTitleTextProperty(tprop)
xyplot3.SetAxisLabelTextProperty(tprop)
xyplot3.GetYAxisActor2D().SetLabelFormat("%4.f")
xyplot3.GetXAxisActor2D().SetLabelFormat("%-#6.2f")
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
ren2.AddViewProp(xyplot3)
renWin.SetSize(790,400)
cam1 = ren1.GetActiveCamera()
cam1.SetClippingRange(3.95297,100)
cam1.SetFocalPoint(8.88908,0.595038,29.3342)
cam1.SetPosition(-12.3332,31.7479,41.2387)
cam1.SetViewUp(0.060772,-0.319905,0.945498)
iren.Initialize()
renWin.Render()
# render the image
#
# prevent the tk window from showing up then start the event loop
# --- end of script --
