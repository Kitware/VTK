#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
from vtkmodules.vtkCommonCore import vtkIntArray
from vtkmodules.vtkCommonDataModel import vtkTable
from vtkmodules.vtkChartsCore import vtkChartXY
from vtkmodules.vtkViewsContext2D import vtkContextView
import vtkmodules.vtkRenderingContextOpenGL2
import vtkmodules.test.Testing
import math

data_2008 = [10822, 10941, 9979, 10370, 9460, 11228, 15093, 12231, 10160, 9816, 9384, 7892]
data_2009 = [9058, 9474, 9979, 9408, 8900, 11569, 14688, 12231, 10294, 9585, 8957, 8590]
data_2010 = [9058, 10941, 9979, 10270, 8900, 11228, 14688, 12231, 10160, 9585, 9384, 8590]

class TestBarGraph(vtkmodules.test.Testing.vtkTest):
    def testBarGraph(self):
        "Test if bar graphs can be built with python"

        # Set up a 2D scene, add an XY chart to it
        view = vtkContextView()
        view.GetRenderer().SetBackground(1.0,1.0,1.0)
        view.GetRenderWindow().SetSize(400,300)
        chart = vtkChartXY()
        view.GetScene().AddItem(chart)

        # Create a table with some points in it
        table = vtkTable()

        arrMonth = vtkIntArray()
        arrMonth.SetName("Month")

        arr2008 = vtkIntArray()
        arr2008.SetName("2008")

        arr2009 = vtkIntArray()
        arr2009.SetName("2009")

        arr2010 = vtkIntArray()
        arr2010.SetName("2010")

        numMonths = 12

        for i in range(0,numMonths):
            arrMonth.InsertNextValue(i + 1)
            arr2008.InsertNextValue(data_2008[i])
            arr2009.InsertNextValue(data_2009[i])
            arr2010.InsertNextValue(data_2010[i])

        table.AddColumn(arrMonth)
        table.AddColumn(arr2008)
        table.AddColumn(arr2009)
        table.AddColumn(arr2010)

        # Now add the line plots with appropriate colors
        line = chart.AddPlot(2)
        line.SetInputData(table,0,1)
        line.SetColor(0,255,0,255)

        line = chart.AddPlot(2)
        line.SetInputData(table,0,2)
        line.SetColor(255,0,0,255)

        line = chart.AddPlot(2)
        line.SetInputData(table,0,3)
        line.SetColor(0,0,255,255)

        view.GetRenderWindow().SetMultiSamples(0)
        view.GetRenderWindow().Render()

        img_file = "TestBarGraph.png"
        vtkmodules.test.Testing.compareImage(view.GetRenderWindow(),vtkmodules.test.Testing.getAbsImagePath(img_file),threshold=25)
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
    vtkmodules.test.Testing.main([(TestBarGraph, 'test')])
