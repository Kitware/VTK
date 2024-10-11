#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
from vtkmodules.vtkCommonCore import (
    vtkFloatArray,
    vtkLookupTable,
)
from vtkmodules.vtkCommonDataModel import vtkTable
from vtkmodules.vtkChartsCore import (
    vtkChart,
    vtkChartXY,
    vtkPlotPoints,
)
from vtkmodules.vtkRenderingContext2D import vtkPen
from vtkmodules.vtkViewsContext2D import vtkContextView
import vtkmodules.vtkRenderingContextOpenGL2
import vtkmodules.test.Testing
import math

class TestLinePlotColors(vtkmodules.test.Testing.vtkTest):
    def testLinePlot(self):
        "Test if colored line plots can be built with python"

        # Set up a 2D scene, add an XY chart to it
        view = vtkContextView()
        view.GetRenderer().SetBackground(1.0, 1.0, 1.0)
        view.GetRenderWindow().SetSize(400, 300)

        chart = vtkChartXY()
        view.GetScene().AddItem(chart)

        # Create a table with some points in it
        arrX = vtkFloatArray()
        arrX.SetName("XAxis")

        arrC = vtkFloatArray()
        arrC.SetName("Cosine")

        arrS = vtkFloatArray()
        arrS.SetName("Sine")

        arrS2 = vtkFloatArray()
        arrS2.SetName("Sine2")

        numPoints = 69
        inc = 7.5 / (numPoints-1)

        for i in range(numPoints):
            arrX.InsertNextValue(i * inc)
            arrC.InsertNextValue(math.cos(i * inc) + 0.0)
            arrS.InsertNextValue(math.sin(i * inc) + 0.0)
            arrS2.InsertNextValue(math.sin(i * inc) + 0.5)

        table = vtkTable()
        table.AddColumn(arrX)
        table.AddColumn(arrC)
        table.AddColumn(arrS)
        table.AddColumn(arrS2)

        # Generate a black-to-red lookup table with fixed alpha
        lut = vtkLookupTable()
        lut.SetValueRange(0.2, 1.0)
        lut.SetSaturationRange(1, 1)
        lut.SetHueRange(0,0)
        lut.SetRampToLinear()
        lut.SetRange(-1,1)
        lut.SetAlpha(0.75)
        lut.Build()

        # Generate a black-to-blue lookup table with alpha range
        lut2 = vtkLookupTable()
        lut2.SetValueRange(0.2, 1.0)
        lut2.SetSaturationRange(1, 1)
        lut2.SetHueRange(0.6667, 0.6667)
        lut2.SetAlphaRange(0.2, 0.8)
        lut2.SetRampToLinear()
        lut2.SetRange(-1,1)
        lut2.Build()

        # Add multiple line plots, setting the colors etc
        line0 = chart.AddPlot(vtkChart.LINE)
        line0.SetInputData(table, 0, 1)
        line0.SetColor(50, 50, 50, 255)
        line0.SetWidth(3.0)
        line0.GetPen().SetLineType(vtkPen.SOLID_LINE)
        line0.SetMarkerStyle(vtkPlotPoints.CIRCLE)
        line0.SetScalarVisibility(1)
        line0.SetLookupTable(lut)
        line0.SelectColorArray(1)

        line1 = chart.AddPlot(vtkChart.LINE)
        line1.SetInputData(table, 0, 2)
        line1.GetPen().SetLineType(vtkPen.NO_PEN)
        line1.SetMarkerStyle(vtkPlotPoints.PLUS)
        line1.SetColor(150, 100, 0, 255)

        line2 = chart.AddPlot(vtkChart.LINE)
        line2.SetInputData(table, 0, 3)
        line2.SetColor(100, 100, 100, 255)
        line2.SetWidth(3.0)
        line2.GetPen().SetLineType(vtkPen.DASH_LINE)
        line2.SetMarkerStyle(vtkPlotPoints.SQUARE)
        line2.ScalarVisibilityOn()
        line2.SetLookupTable(lut2)
        line2.SelectColorArray("Sine")

        chart.SetShowLegend(True)

        view.GetRenderWindow().SetMultiSamples(0)
        view.GetRenderWindow().Render()

        img_file = "TestLinePlotColors.png"
        vtkmodules.test.Testing.compareImage(view.GetRenderWindow(),vtkmodules.test.Testing.getAbsImagePath(img_file))
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
    vtkmodules.test.Testing.main([(TestLinePlotColors, 'test')])
