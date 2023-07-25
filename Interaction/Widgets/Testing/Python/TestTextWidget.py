#!/usr/bin/env python
# -*- coding: utf-8 -*-



from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkInteractionWidgets import vtkTextWidget
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class TestTextWidget(vtkmodules.test.Testing.vtkTest):

    def testTextWidget(self):

        # Create fake data
        #
        ss = vtkSphereSource()
        mapper = vtkPolyDataMapper()
        mapper.SetInputConnection(ss.GetOutputPort())
        actor = vtkActor()
        actor.SetMapper(mapper)

        # Create the RenderWindow, Renderer and both Actors
        #
        ren = vtkRenderer()
        renWin = vtkRenderWindow()
        renWin.AddRenderer(ren)
        iRen = vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin)

        ren.AddActor(actor)
        ren.SetBackground(0.1, 0.2, 0.4)
        renWin.SetSize(300, 300)

        iRen.Initialize()
        renWin.Render()

        widget = vtkTextWidget()
        widget.SetInteractor(iRen)
        widget.On()
        widget.GetTextActor().SetInput("This is a test")
        widget.GetTextActor().GetTextProperty().SetColor(0, 1, 0)
        widget.GetRepresentation().GetPositionCoordinate().SetValue(.15, .15)
        widget.GetRepresentation().GetPosition2Coordinate().SetValue(.7, .2)


        # Add the actors to the renderer, set the background and size
        #
        ren.AddActor(actor)
        ren.SetBackground(.1, .2, .4)

        iRen.Initialize()
        renWin.Render()

        # render and interact with data

        renWin.Render()


        img_file = "TestTextWidget.png"
        vtkmodules.test.Testing.compareImage(iRen.GetRenderWindow(), vtkmodules.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
     vtkmodules.test.Testing.main([(TestTextWidget, 'test')])
