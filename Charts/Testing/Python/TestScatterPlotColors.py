#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Run this test like so:
# vtkpython TestScatterPlotColors.py  -D $VTK_DATA_ROOT \
# -B $VTK_DATA_ROOT/Baseline/Charts/

import os
import vtk
import vtk.test.Testing
import math

class TestScatterPlotColors(vtk.test.Testing.vtkTest):
    def testLinePlot(self):
        "Test if colored scatter plots can be built with python"

        # Set up a 2D scene, add an XY chart to it
        view = vtk.vtkContextView()
        view.GetRenderer().SetBackground(1.0, 1.0, 1.0)
        view.GetRenderWindow().SetSize(400, 300)

        chart = vtk.vtkChartXY()
        chart.SetShowLegend(True)
        view.GetScene().AddItem(chart)

        # Create a table with some points in it
        arrX = vtk.vtkFloatArray()
        arrX.SetName("XAxis")

        arrC = vtk.vtkFloatArray()
        arrC.SetName("Cosine")

        arrS = vtk.vtkFloatArray()
        arrS.SetName("Sine")

        arrS2 = vtk.vtkFloatArray()
        arrS2.SetName("Tan")

        numPoints = 40
        inc = 7.5 / (numPoints-1)

        for i in range(numPoints):
            arrX.InsertNextValue(i * inc)
            arrC.InsertNextValue(math.cos(i * inc) + 0.0)
            arrS.InsertNextValue(math.sin(i * inc) + 0.0)
            arrS2.InsertNextValue(math.tan(i * inc) + 0.5)

        table = vtk.vtkTable()
        table.AddColumn(arrX)
        table.AddColumn(arrC)
        table.AddColumn(arrS)
        table.AddColumn(arrS2)

        # Generate a black-to-red lookup table with fixed alpha
        lut = vtk.vtkLookupTable()
        lut.SetValueRange(0.2, 1.0)
        lut.SetSaturationRange(1, 1)
        lut.SetHueRange(0,0)
        lut.SetRampToLinear()
        lut.SetRange(-1,1)
        lut.SetAlpha(0.75)
        lut.Build()

        # Generate a black-to-blue lookup table with alpha range
        lut2 = vtk.vtkLookupTable()
        lut2.SetValueRange(0.2, 1.0)
        lut2.SetSaturationRange(1, 1)
        lut2.SetHueRange(0.6667, 0.6667)
        lut2.SetAlphaRange(0.4, 0.8)
        lut2.SetRampToLinear()
        lut2.SetRange(-1,1)
        lut2.Build()

        # Add multiple line plots, setting the colors etc
        points0 = chart.AddPlot(vtk.vtkChart.POINTS)
        points0.SetInput(table, 0, 1)
        points0.SetColor(0, 0, 0, 255)
        points0.SetWidth(1.0)
        points0.SetMarkerStyle(vtk.vtkPlotPoints.CROSS)

        points1 = chart.AddPlot(vtk.vtkChart.POINTS)
        points1.SetInput(table, 0, 2)
        points1.SetColor(0, 0, 0, 255)
        points1.SetMarkerStyle(vtk.vtkPlotPoints.DIAMOND)
        points1.SetScalarVisibility(1)
        points1.SetLookupTable(lut)
        points1.SelectColorArray(1)

        points2 = chart.AddPlot(vtk.vtkChart.POINTS)
        points2.SetInput(table, 0, 3)
        points2.SetColor(0, 0, 0, 255)
        points2.ScalarVisibilityOn()
        points2.SetLookupTable(lut2)
        points2.SelectColorArray("Cosine")
        points2.SetWidth(4.0)

        view.GetRenderWindow().SetMultiSamples(0)
        view.GetRenderWindow().Render()

        img_file = "TestScatterPlotColors.png"
        vtk.test.Testing.compareImage(view.GetRenderWindow(),vtk.test.Testing.getAbsImagePath(img_file),threshold=25)
        vtk.test.Testing.interact()

if __name__ == "__main__":
    vtk.test.Testing.main([(TestScatterPlotColors, 'test')])
