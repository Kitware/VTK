#!/usr/bin/env python

# Run this test like so:
# vtkpython TestStackedPlot.py  -D $VTK_DATA_ROOT \
# -B $VTK_DATA_ROOT/Baseline/Charts/

import os
import vtk
import vtk.test.Testing
import math

book =       [5675, 5902, 6388, 5990, 5575, 7393, 9878, 8082, 6417, 5946, 5526, 5166];
new_popular = [701,  687,  736,  696,  750,  814,  923,  860,  786,  735,  680,  741];
periodical =  [184,  176,  166,  131,  171,  191,  231,  166,  197,  162,  152,  143];
audiobook =   [903, 1038,  987, 1073, 1144, 1203, 1173, 1196, 1213, 1076,  926,  874];
video =      [1524, 1565, 1627, 1445, 1179, 1816, 2293, 1811, 1588, 1561, 1542, 1563];

class TestStackedPlot(vtk.test.Testing.vtkTest):
    def testStackedPlot(self):
        "Test if stacked plots can be built with python"

        # Set up a 2D scene, add an XY chart to it
        view = vtk.vtkContextView()
        view.GetRenderer().SetBackground(1.0,1.0,1.0)
        view.GetRenderWindow().SetSize(400,300)
        chart = vtk.vtkChartXY()
        view.GetScene().AddItem(chart)

        # Create a table with some points in it
        table = vtk.vtkTable()

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
            arrMonth.InsertNextValue(i + 1)
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

        # Now add the line plots with appropriate colors

        # Books
        line = chart.AddPlot(3)
        line.SetInput(table,0,1)
        line.SetColor(120,120,254,255)

        # New / Popular
        line = chart.AddPlot(3)
        line.SetInput(table,0,2)
        line.SetColor(254,118,118,255)

        # Periodical
        line = chart.AddPlot(3)
        line.SetInput(table,0,3)
        line.SetColor(170,170,254,255)

        # Audiobook
        line = chart.AddPlot(3)
        line.SetInput(table,0,4)
        line.SetColor(91,91,254,255)

        # Video
        line = chart.AddPlot(3)
        line.SetInput(table,0,5)
        line.SetColor(253,158,158,255)

        view.GetRenderWindow().SetMultiSamples(0)
        # view.GetRenderWindow().GetInteractor().Start()

        img_file = "TestStackedPlot.png"
        img_file2 = "TestStackedPlot0Hidden.png"
        vtk.test.Testing.compareImage(view.GetRenderWindow(),vtk.test.Testing.getAbsImagePath(img_file),threshold=25)
        vtk.test.Testing.interact()
        chart.GetPlot(0).SetVisible(False)
        vtk.test.Testing.compareImage(view.GetRenderWindow(),vtk.test.Testing.getAbsImagePath(img_file2),threshold=25)
        vtk.test.Testing.interact()

if __name__ == "__main__":
    vtk.test.Testing.main([(TestStackedPlot, 'test')])
