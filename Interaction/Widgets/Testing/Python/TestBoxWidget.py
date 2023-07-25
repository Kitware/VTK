#!/usr/bin/env python
# -*- coding: utf-8 -*-



from vtkmodules.vtkCommonDataModel import vtkPlanes
from vtkmodules.vtkFiltersCore import (
    vtkAppendPolyData,
    vtkClipPolyData,
    vtkGlyph3D,
)
from vtkmodules.vtkFiltersSources import (
    vtkConeSource,
    vtkSphereSource,
)
from vtkmodules.vtkInteractionWidgets import vtkBoxWidget
from vtkmodules.vtkRenderingCore import (
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
from vtkmodules.vtkRenderingLOD import vtkLODActor
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class TestBoxWidget(vtkmodules.test.Testing.vtkTest):

    def testBoxWidget(self):

        # Demonstrate how to use the vtkBoxWidget.
        # This script uses a 3D box widget to define a "clipping box" to clip some
        # simple geometry (a mace). Make sure that you hit the "W" key to activate the widget.

        # create a sphere source
        #
        sphere = vtkSphereSource()
        cone = vtkConeSource()
        glyph = vtkGlyph3D()
        glyph.SetInputConnection(sphere.GetOutputPort())
        glyph.SetSourceConnection(cone.GetOutputPort())
        glyph.SetVectorModeToUseNormal()
        glyph.SetScaleModeToScaleByVector()
        glyph.SetScaleFactor(0.25)
        apd = vtkAppendPolyData()
        apd.AddInputConnection(glyph.GetOutputPort())
        apd.AddInputConnection(sphere.GetOutputPort())
        maceMapper = vtkPolyDataMapper()
        maceMapper.SetInputConnection(apd.GetOutputPort())
        maceActor = vtkLODActor()
        maceActor.SetMapper(maceMapper)
        maceActor.VisibilityOn()

        planes = vtkPlanes()
        clipper = vtkClipPolyData()
        clipper.SetInputConnection(apd.GetOutputPort())
        clipper.SetClipFunction(planes)
        clipper.InsideOutOn()
        selectMapper = vtkPolyDataMapper()
        selectMapper.SetInputConnection(clipper.GetOutputPort())
        selectActor = vtkLODActor()
        selectActor.SetMapper(selectMapper)
        selectActor.GetProperty().SetColor(0, 1, 0)
        selectActor.VisibilityOff()
        selectActor.SetScale(1.01, 1.01, 1.01)

        # Create the RenderWindow, Renderer and both Actors
        #
        ren = vtkRenderer()
        renWin = vtkRenderWindow()
        renWin.AddRenderer(ren)

        iRen = vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        boxWidget = vtkBoxWidget()
        boxWidget.SetInteractor(iRen)

        ren.AddActor(maceActor)
        ren.AddActor(selectActor)

        # Add the actors to the renderer, set the background and size
        #
        ren.SetBackground(0.1, 0.2, 0.4)
        renWin.SetSize(300, 300)

        def SelectPolygons(widget, event_string):
            '''
            The callback takes two parameters.
            Parameters:
              widget - the object that generates the event.
              event_string - the event name (which is a string).
            '''
            boxWidget, selectActor
            boxWidget.GetPlanes(planes)
            selectActor.VisibilityOn()

        # place the interactor initially
        boxWidget.SetInputConnection(glyph.GetOutputPort())
        boxWidget.PlaceWidget()
        boxWidget.AddObserver("EndInteractionEvent", SelectPolygons)

        # render and interact with data

        renWin.Render()

        img_file = "TestBoxWidget.png"
        vtkmodules.test.Testing.compareImage(iRen.GetRenderWindow(), vtkmodules.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
     vtkmodules.test.Testing.main([(TestBoxWidget, 'test')])
