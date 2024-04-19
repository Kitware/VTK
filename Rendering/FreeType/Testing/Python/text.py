#!/usr/bin/env python
from vtkmodules.vtkFiltersSources import vtkTextSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
from vtkmodules.vtkRenderingFreeType import vtkVectorText
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Created oriented text
text0Source = vtkTextSource()
text0Source.SetText("Text Source with Scalars (default)")
text0Mapper = vtkPolyDataMapper()
text0Mapper.SetInputConnection(text0Source.GetOutputPort())
text0Actor = vtkActor()
text0Actor.SetMapper(text0Mapper)
text0Actor.SetScale(.1,.1,.1)
text0Actor.AddPosition(0,2,0)
text1Source = vtkTextSource()
text1Source.SetText("Text Source with Scalars")
text1Source.SetForegroundColor(1,0,0)
text1Source.SetBackgroundColor(1,1,1)
text1Mapper = vtkPolyDataMapper()
text1Mapper.SetInputConnection(text1Source.GetOutputPort())
text1Actor = vtkActor()
text1Actor.SetMapper(text1Mapper)
text1Actor.SetScale(.1,.1,.1)
text2Source = vtkTextSource()
text2Source.SetText("Text Source without Scalars")
text2Source.BackingOff()
text2Mapper = vtkPolyDataMapper()
text2Mapper.SetInputConnection(text2Source.GetOutputPort())
text2Mapper.ScalarVisibilityOff()
text2Actor = vtkActor()
text2Actor.SetMapper(text2Mapper)
text2Actor.GetProperty().SetColor(1,1,0)
text2Actor.SetScale(.1,.1,.1)
text2Actor.AddPosition(0,-2,0)
text3Source = vtkVectorText()
text3Source.SetText("Vector Text")
text3Mapper = vtkPolyDataMapper()
text3Mapper.SetInputConnection(text3Source.GetOutputPort())
text3Mapper.ScalarVisibilityOff()
text3Actor = vtkActor()
text3Actor.SetMapper(text3Mapper)
text3Actor.GetProperty().SetColor(.1,1,0)
text3Actor.AddPosition(0,-4,0)
# create graphics stuff
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetSize(350,100)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren1.AddActor(text0Actor)
ren1.AddActor(text1Actor)
ren1.AddActor(text2Actor)
ren1.AddActor(text3Actor)
ren1.ResetCamera()
ren1.GetActiveCamera().Zoom(3)
ren1.SetBackground(.1,.2,.4)
renWin.Render()
iren.Initialize()
# --- end of script --
