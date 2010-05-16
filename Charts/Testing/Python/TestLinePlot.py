#!/usr/bin/env python

# Run this test like so:
# vtkpython TestLinePlot.py  -D $VTK_DATA_ROOT \
# -B $VTK_DATA_ROOT/Baseline/Charts/

import os
import vtk
import vtk.test.Testing
import math

class TestLinePlot(vtk.test.Testing.vtkTest):
    def testLinePlot(self):
        "Test if line plots can be built with python"

        # Set up a 2D scene, add an XY chart to it
        view = vtk.vtkContextView()
        view.GetRenderer().SetBackground(1.0,1.0,1.0)
        view.GetRenderWindow().SetSize(400,300)
        chart = vtk.vtkChartXY()
        view.GetScene().AddItem(chart)

        # Create a table with some points in it
        table = vtk.vtkTable()

        arrX = vtk.vtkFloatArray()
        arrX.SetName("X Axis")

        arrC = vtk.vtkFloatArray()
        arrC.SetName("Cosine")

        arrS = vtk.vtkFloatArray()
        arrS.SetName("Sine")

        arrS2 = vtk.vtkFloatArray()
        arrS2.SetName("Sine2")

        numPoints = 69
        inc = 7.5 / (numPoints - 1)

        for i in range(0,numPoints):
            arrX.InsertNextValue(i*inc)
            arrC.InsertNextValue(math.cos(i * inc) + 0.0)
            arrS.InsertNextValue(math.sin(i * inc) + 0.0)
            arrS2.InsertNextValue(math.sin(i * inc) + 0.5)

        table.AddColumn(arrX)
        table.AddColumn(arrC)
        table.AddColumn(arrS)
        table.AddColumn(arrS2)

        # Now add the line plots with appropriate colors
        line = chart.AddPlot(0)
        line.SetInput(table,0,1)
        line.SetColor(0,255,0,255)
        line.SetWidth(1.0)

        line = chart.AddPlot(0)
        line.SetInput(table,0,2)
        line.SetColor(255,0,0,255);
        line.SetWidth(5.0)

        line = chart.AddPlot(0)
        line.SetInput(table,0,3)
        line.SetColor(0,0,255,255);
        line.SetWidth(4.0)

        view.GetRenderWindow().SetMultiSamples(0)

        img_file = "TestLinePlot.png"
        vtk.test.Testing.compareImage(view.GetRenderWindow(),vtk.test.Testing.getAbsImagePath(img_file),threshold=25)
        vtk.test.Testing.interact()

if __name__ == "__main__":
    vtk.test.Testing.main([(TestLinePlot, 'test')])
