#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
from vtkmodules.vtkCommonCore import vtkFloatArray
from vtkmodules.vtkCommonDataModel import vtkTable
from vtkmodules.vtkChartsCore import vtkChartXY
from vtkmodules.vtkViewsContext2D import vtkContextView
import vtkmodules.vtkRenderingContextOpenGL2
import vtkmodules.test.Testing
import math

class TestLinePlot(vtkmodules.test.Testing.vtkTest):
    def testLinePlot(self):
        "Test if line plots can be built with python"

        # Set up a 2D scene, add an XY chart to it
        view = vtkContextView()
        view.GetRenderer().SetBackground(1.0,1.0,1.0)
        view.GetRenderWindow().SetSize(400,300)
        chart = vtkChartXY()
        view.GetScene().AddItem(chart)

        # Create a table with some points in it
        table = vtkTable()

        arrX = vtkFloatArray()
        arrX.SetName("X Axis")

        arrC = vtkFloatArray()
        arrC.SetName("Cosine")

        arrS = vtkFloatArray()
        arrS.SetName("Sine")

        arrS2 = vtkFloatArray()
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
        line.SetInputData(table,0,1)
        line.SetColor(0,255,0,255)
        line.SetWidth(1.0)

        line = chart.AddPlot(0)
        line.SetInputData(table,0,2)
        line.SetColor(255,0,0,255)
        line.SetWidth(5.0)

        line = chart.AddPlot(0)
        line.SetInputData(table,0,3)
        line.SetColor(0,0,255,255)
        line.SetWidth(4.0)

        view.GetRenderWindow().SetMultiSamples(0)
        view.GetRenderWindow().Render()

        img_file = "TestLinePlot.png"
        vtkmodules.test.Testing.compareImage(view.GetRenderWindow(),vtkmodules.test.Testing.getAbsImagePath(img_file))
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
    vtkmodules.test.Testing.main([(TestLinePlot, 'test')])
