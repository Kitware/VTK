#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Run this test like so:
# vtkpython TestLinePlotColors.py  -D $VTK_DATA_ROOT \
# -B $VTK_DATA_ROOT/Baseline/Charts/

import os
import vtk
import vtk.test.Testing
import math

class TestLinePlotColors(vtk.test.Testing.vtkTest):
    def testLinePlot(self):
        "Test if colored line plots can be built with python"

        # Set up a 2D scene, add an XY chart to it
        view = vtk.vtkContextView()
        view.GetRenderer().SetBackground(1.0, 1.0, 1.0)
        view.GetRenderWindow().SetSize(400, 300)

        chart = vtk.vtkChartXY()
        view.GetScene().AddItem(chart)

        # Create a table with some points in it
        arrX = vtk.vtkFloatArray()
        arrX.SetName("XAxis")

        arrC = vtk.vtkFloatArray()
        arrC.SetName("Cosine")

        arrS = vtk.vtkFloatArray()
        arrS.SetName("Sine")

        arrS2 = vtk.vtkFloatArray()
        arrS2.SetName("Sine2")

        numPoints = 69
        inc = 7.5 / (numPoints-1)

        for i in range(numPoints):
            arrX.InsertNextValue(i * inc)
            arrC.InsertNextValue(math.cos(i * inc) + 0.0)
            arrS.InsertNextValue(math.sin(i * inc) + 0.0)
            arrS2.InsertNextValue(math.sin(i * inc) + 0.5)

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
        lut2.SetAlphaRange(0.2, 0.8)
        lut2.SetRampToLinear()
        lut2.SetRange(-1,1)
        lut2.Build()

        # Add multiple line plots, setting the colors etc
        line0 = chart.AddPlot(vtk.vtkChart.LINE)
        line0.SetInput(table, 0, 1)
        line0.SetColor(50, 50, 50, 255)
        line0.SetWidth(3.0)
        line0.GetPen().SetLineType(vtk.vtkPen.SOLID_LINE)
        line0.SetMarkerStyle(vtk.vtkPlotPoints.CIRCLE)
        line0.SetScalarVisibility(1)
        line0.SetLookupTable(lut)
        line0.SelectColorArray(1)

        line1 = chart.AddPlot(vtk.vtkChart.LINE)
        line1.SetInput(table, 0, 2)
        line1.GetPen().SetLineType(vtk.vtkPen.NO_PEN)
        line1.SetMarkerStyle(vtk.vtkPlotPoints.PLUS)
        line1.SetColor(150, 100, 0, 255)

        line2 = chart.AddPlot(vtk.vtkChart.LINE)
        line2.SetInput(table, 0, 3)
        line2.SetColor(100, 100, 100, 255)
        line2.SetWidth(3.0)
        line2.GetPen().SetLineType(vtk.vtkPen.DASH_LINE)
        line2.SetMarkerStyle(vtk.vtkPlotPoints.SQUARE)
        line2.ScalarVisibilityOn()
        line2.SetLookupTable(lut2)
        line2.SelectColorArray("Sine")

        chart.SetShowLegend(True)

        view.GetRenderWindow().SetMultiSamples(0)
        view.GetRenderWindow().Render()

        img_file = "TestLinePlotColors.png"
        vtk.test.Testing.compareImage(view.GetRenderWindow(),vtk.test.Testing.getAbsImagePath(img_file),threshold=25)
        vtk.test.Testing.interact()

if __name__ == "__main__":
    vtk.test.Testing.main([(TestLinePlotColors, 'test')])
