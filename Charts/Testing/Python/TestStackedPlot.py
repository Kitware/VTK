#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Run this test like so:
# vtkpython TestStackedPlot.py  -D $VTK_DATA_ROOT \
# -B $VTK_DATA_ROOT/Baseline/Charts/

import os
import vtk
import vtk.test.Testing
import math

month_labels =  ["Jan", "Feb", "Mar", "Apr", "May", "Jun",
               "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]
book =       [5675, 5902, 6388, 5990, 5575, 7393, 9878, 8082, 6417, 5946, 5526, 5166]
new_popular = [701,  687,  736,  696,  750,  814,  923,  860,  786,  735,  680,  741]
periodical =  [184,  176,  166,  131,  171,  191,  231,  166,  197,  162,  152,  143]
audiobook =   [903, 1038,  987, 1073, 1144, 1203, 1173, 1196, 1213, 1076,  926,  874]
video =      [1524, 1565, 1627, 1445, 1179, 1816, 2293, 1811, 1588, 1561, 1542, 1563]

class TestStackedPlot(vtk.test.Testing.vtkTest):
    def testStackedPlot(self):
        "Test if stacked plots can be built with python"

        # Set up a 2D scene, add an XY chart to it
        view = vtk.vtkContextView()
        view.GetRenderer().SetBackground(1.0,1.0,1.0)
        view.GetRenderWindow().SetSize(400,300)
        chart = vtk.vtkChartXY()
        view.GetScene().AddItem(chart)

        # Create a table with some data in it
        table = vtk.vtkTable()

        arrMonthLabels = vtk.vtkStringArray()
        arrMonthPositions = vtk.vtkDoubleArray()

        arrMonth = vtk.vtkIntArray()
        arrMonth.SetName("Month")

        arrBooks = vtk.vtkIntArray()
        arrBooks.SetName("Books")

        arrNew = vtk.vtkIntArray()
        arrNew.SetName("New / Popular")

        arrPeriodical = vtk.vtkIntArray()
        arrPeriodical.SetName("Periodical")

        arrAudiobook = vtk.vtkIntArray()
        arrAudiobook.SetName("Audiobook")

        arrVideo = vtk.vtkIntArray()
        arrVideo.SetName("Video")

        numMonths = 12

        for i in range(0,numMonths):
            arrMonthLabels.InsertNextValue(month_labels[i])
            arrMonthPositions.InsertNextValue(float(i))

            arrMonth.InsertNextValue(i)
            arrBooks.InsertNextValue(book[i])
            arrNew.InsertNextValue(new_popular[i])
            arrPeriodical.InsertNextValue(periodical[i])
            arrAudiobook.InsertNextValue(audiobook[i])
            arrVideo.InsertNextValue(video[i])

        table.AddColumn(arrMonth)
        table.AddColumn(arrBooks)
        table.AddColumn(arrNew)
        table.AddColumn(arrPeriodical)
        table.AddColumn(arrAudiobook)
        table.AddColumn(arrVideo)

        # Set up the X Labels

        chart.GetAxis(1).SetTickLabels(arrMonthLabels)
        chart.GetAxis(1).SetTickPositions(arrMonthPositions)
        chart.GetAxis(1).SetMaximum(11)

        # Create the stacked plot
        stack = chart.AddPlot(3)
        stack.SetUseIndexForXSeries(True)
        stack.SetInput(table)
        stack.SetInputArray(1,"Books")
        stack.SetInputArray(2,"New / Popular")
        stack.SetInputArray(3,"Periodical")
        stack.SetInputArray(4,"Audiobook")
        stack.SetInputArray(5,"Video")

        # Set up a nice color series
        colorSeries = vtk.vtkColorSeries()
        colorSeries.SetColorScheme(2)
        stack.SetColorSeries(colorSeries)

        view.GetRenderWindow().SetMultiSamples(0)
        view.GetRenderWindow().Render()

        img_file = "TestStackedPlot.png"
        vtk.test.Testing.compareImage(view.GetRenderWindow(),
                                      vtk.test.Testing.getAbsImagePath(img_file),
                                      threshold=25)
        vtk.test.Testing.interact()

if __name__ == "__main__":
    vtk.test.Testing.main([(TestStackedPlot, 'test')])
