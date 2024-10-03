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
from vtkmodules.vtkViewsContext2D import vtkContextView
import vtkmodules.vtkRenderingContextOpenGL2
import vtkmodules.test.Testing
import math

class TestScatterPlotColors(vtkmodules.test.Testing.vtkTest):
    def testLinePlot(self):
        "Test if colored scatter plots can be built with python"

        # Set up a 2D scene, add an XY chart to it
        view = vtkContextView()
        view.GetRenderer().SetBackground(1.0, 1.0, 1.0)
        view.GetRenderWindow().SetSize(400, 300)

        chart = vtkChartXY()
        chart.SetShowLegend(True)
        view.GetScene().AddItem(chart)

        # Create a table with some points in it
        arrX = vtkFloatArray()
        arrX.SetName("XAxis")

        arrC = vtkFloatArray()
        arrC.SetName("Cosine")

        arrS = vtkFloatArray()
        arrS.SetName("Sine")

        arrS2 = vtkFloatArray()
        arrS2.SetName("Tan")

        numPoints = 40
        inc = 7.5 / (numPoints-1)

        for i in range(numPoints):
            arrX.InsertNextValue(i * inc)
            arrC.InsertNextValue(math.cos(i * inc) + 0.0)
            arrS.InsertNextValue(math.sin(i * inc) + 0.0)
            arrS2.InsertNextValue(math.tan(i * inc) + 0.5)

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
        lut2.SetAlphaRange(0.4, 0.8)
        lut2.SetRampToLinear()
        lut2.SetRange(-1,1)
        lut2.Build()

        # Add multiple line plots, setting the colors etc
        points0 = chart.AddPlot(vtkChart.POINTS)
        points0.SetInputData(table, 0, 1)
        points0.SetColor(0, 0, 0, 255)
        points0.SetWidth(1.0)
        points0.SetMarkerStyle(vtkPlotPoints.CROSS)

        points1 = chart.AddPlot(vtkChart.POINTS)
        points1.SetInputData(table, 0, 2)
        points1.SetColor(0, 0, 0, 255)
        points1.SetMarkerStyle(vtkPlotPoints.DIAMOND)
        points1.SetScalarVisibility(1)
        points1.SetLookupTable(lut)
        points1.SelectColorArray(1)

        points2 = chart.AddPlot(vtkChart.POINTS)
        points2.SetInputData(table, 0, 3)
        points2.SetColor(0, 0, 0, 255)
        points2.ScalarVisibilityOn()
        points2.SetLookupTable(lut2)
        points2.SelectColorArray("Cosine")
        points2.SetWidth(4.0)

        view.GetRenderWindow().SetMultiSamples(0)
        view.GetRenderWindow().Render()

        img_file = "TestScatterPlotColors.png"
        vtkmodules.test.Testing.compareImage(view.GetRenderWindow(),vtkmodules.test.Testing.getAbsImagePath(img_file))
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
    vtkmodules.test.Testing.main([(TestScatterPlotColors, 'test')])
