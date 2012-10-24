#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# A script to test the threshold filter.
# Values above 2000 are set to 255.
# Values below 2000 are set to 0.
# Image pipeline
imageCanvas = vtk.vtkImageCanvasSource2D()
imageCanvas.SetScalarTypeToUnsignedChar()
imageCanvas.SetNumberOfScalarComponents(3)
imageCanvas.SetExtent(0,300,0,300,0,0)
# background black
imageCanvas.SetDrawColor(0)
imageCanvas.FillBox(0,511,0,511)
jreader = vtk.vtkJPEGReader()
jreader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/beach.jpg")
jreader.Update()
imageCanvas.DrawImage(100,100,jreader.GetOutput(),0,0,300,300)
imageCanvas.DrawImage(0,100,jreader.GetOutput())
imageCanvas.DrawImage(100,0,jreader.GetOutput(),0,0,300,300)
imageCanvas.DrawImage(0,0,jreader.GetOutput(),50,50,100,100)
imageCanvas.DrawImage(10,10,jreader.GetOutput(),50,50,100,100)
imageCanvas.DrawImage(20,20,jreader.GetOutput(),50,50,100,100)
imageCanvas.DrawImage(30,30,jreader.GetOutput(),50,50,100,100)
imageCanvas.DrawImage(40,40,jreader.GetOutput(),50,50,100,100)
imageCanvas.DrawImage(50,50,jreader.GetOutput(),50,50,100,100)
imageCanvas.DrawImage(60,60,jreader.GetOutput(),50,50,100,100)
imageCanvas.DrawImage(70,70,jreader.GetOutput(),50,50,100,100)
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(imageCanvas.GetOutputPort())
viewer.SetColorWindow(255)
viewer.SetColorLevel(128)
viewer.Render()
# --- end of script --
