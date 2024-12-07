#!/usr/bin/env python
# -*- coding: utf-8 -*-



import sys
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
from vtkmodules.vtkInteractionWidgets import (
    vtkBoxRepresentation,
    vtkBoxWidget2,
)
from vtkmodules.vtkRenderingCore import (
    vtkInteractorEventRecorder,
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

class TestInteractorEventRecorder(vtkmodules.test.Testing.vtkTest):

    def testInteractorEventRecorder(self):

        # Demonstrate how to use the vtkInteractorEventRecorder to play back some
        # events.

        # Create a mace out of filters.
        #
        sphere = vtkSphereSource()
        cone = vtkConeSource()
        glyph = vtkGlyph3D()
        glyph.SetInputConnection(sphere.GetOutputPort())
        glyph.SetSourceConnection(cone.GetOutputPort())
        glyph.SetVectorModeToUseNormal()
        glyph.SetScaleModeToScaleByVector()
        glyph.SetScaleFactor(0.25)

        # The sphere and spikes are appended into a single polydata. This just makes things
        # simpler to manage.
        apd = vtkAppendPolyData()
        apd.AddInputConnection(glyph.GetOutputPort())
        apd.AddInputConnection(sphere.GetOutputPort())
        maceMapper = vtkPolyDataMapper()
        maceMapper.SetInputConnection(apd.GetOutputPort())
        maceActor = vtkLODActor()
        maceActor.SetMapper(maceMapper)
        maceActor.VisibilityOn()

        # This portion of the code clips the mace with the vtkPlanes implicit function.
        # The clipped region is colored green.
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
        #iren = vtkRenderWindowInteractor()
        #iren.SetRenderWindow(renWin)
        iRen.AddObserver("ExitEvent", sys.exit)

        # The SetInteractor method is how 3D widgets are associated with the render
        # window interactor. Internally, SetInteractor sets up a bunch of callbacks
        # using the Command/Observer mechanism (AddObserver()).
        ren.AddActor(maceActor)
        ren.AddActor(selectActor)

        # Add the actors to the renderer, set the background and size
        #
        ren.SetBackground(0.1, 0.2, 0.4)
        renWin.SetSize(300, 300)

        # This does the actual work: updates the vtkPlanes implicit function.
        # This in turn causes the pipeline to update.
        def SelectPolygons(widget, event_string):
            '''
            The callback takes two parameters.
            Parameters:
              widget - the object that generates the event.
              event_string - the event name (which is a string).
            '''
            boxRep.GetPlanes(planes)
            selectActor.VisibilityOn()

        # Place the interactor initially. The input to a 3D widget is used to
        # initially position and scale the widget. The EndInteractionEvent is
        # observed which invokes the SelectPolygons callback.
        boxRep = vtkBoxRepresentation()
        boxRep.SetPlaceFactor(0.75)
        boxRep.PlaceWidget(glyph.GetOutput().GetBounds())
        boxWidget = vtkBoxWidget2()
        boxWidget.SetInteractor(iRen)
        boxWidget.SetRepresentation(boxRep)
        boxWidget.AddObserver("EndInteractionEvent", SelectPolygons)
        boxWidget.SetPriority(1)

        # record events
        recorder = vtkInteractorEventRecorder()
        recorder.SetInteractor(iRen)
        recorder.SetFileName(VTK_DATA_ROOT + "/Data/EventRecording.log")

        # render the image
        iRen.Initialize()
        renWin.Render()
        #recorder.Record()

        recorder.Play()
        recorder.Off()

        # render and interact with data

        renWin.Render()

        img_file = "TestInteractorEventRecorder.png"
        vtkmodules.test.Testing.compareImage(iRen.GetRenderWindow(), vtkmodules.test.Testing.getAbsImagePath(img_file))
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
     vtkmodules.test.Testing.main([(TestInteractorEventRecorder, 'test')])
