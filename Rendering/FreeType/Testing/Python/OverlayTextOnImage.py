#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

#
# display text over an image
#
ellipse = vtk.vtkImageEllipsoidSource()
mapImage = vtk.vtkImageMapper()
mapImage.SetInputConnection(ellipse.GetOutputPort())
mapImage.SetColorWindow(255)
mapImage.SetColorLevel(127.5)
img = vtk.vtkActor2D()
img.SetMapper(mapImage)
mapText = vtk.vtkTextMapper()
mapText.SetInput("Text Overlay")
mapText.GetTextProperty().SetFontSize(15)
mapText.GetTextProperty().SetColor(0,1,1)
mapText.GetTextProperty().BoldOn()
mapText.GetTextProperty().ShadowOn()
txt = vtk.vtkActor2D()
txt.SetMapper(mapText)
txt.SetPosition(138,128)
ren1 = vtk.vtkRenderer()
ren1.AddActor2D(img)
ren1.AddActor2D(txt)
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
renWin.Render()
iren.Initialize()
# --- end of script --
