#!/usr/bin/env python
from vtkmodules.vtkImagingSources import vtkImageEllipsoidSource
from vtkmodules.vtkRenderingCore import (
    vtkActor2D,
    vtkImageMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
    vtkTextMapper,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

#
# display text over an image
#
ellipse = vtkImageEllipsoidSource()
mapImage = vtkImageMapper()
mapImage.SetInputConnection(ellipse.GetOutputPort())
mapImage.SetColorWindow(255)
mapImage.SetColorLevel(127.5)
img = vtkActor2D()
img.SetMapper(mapImage)
mapText = vtkTextMapper()
mapText.SetInput("Text Overlay")
mapText.GetTextProperty().SetFontSize(15)
mapText.GetTextProperty().SetColor(0,1,1)
mapText.GetTextProperty().BoldOn()
mapText.GetTextProperty().ShadowOn()
txt = vtkActor2D()
txt.SetMapper(mapText)
txt.SetPosition(138,128)
ren1 = vtkRenderer()
ren1.AddActor2D(img)
ren1.AddActor2D(txt)
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
renWin.Render()
iren.Initialize()
# --- end of script --
