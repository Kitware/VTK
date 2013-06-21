#!/usr/bin/env python
# -*- coding: utf-8 -*-

'''
=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNamedColorsIntegration.py

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================
'''

import sys
import vtk
import vtk.test.Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class TestInteractorEventRecorder(vtk.test.Testing.vtkTest):

    def testInteractorEventRecorder(self):

        # Demonstrate how to use the vtkInteractorEventRecorder to play back some
        # events.

        # Create a mace out of filters.
        #
        sphere = vtk.vtkSphereSource()
        cone = vtk.vtkConeSource()
        glyph = vtk.vtkGlyph3D()
        glyph.SetInputConnection(sphere.GetOutputPort())
        glyph.SetSourceConnection(cone.GetOutputPort())
        glyph.SetVectorModeToUseNormal()
        glyph.SetScaleModeToScaleByVector()
        glyph.SetScaleFactor(0.25)

        # The sphere and spikes are appended into a single polydata. This just makes things
        # simpler to manage.
        apd = vtk.vtkAppendPolyData()
        apd.AddInputConnection(glyph.GetOutputPort())
        apd.AddInputConnection(sphere.GetOutputPort())
        maceMapper = vtk.vtkPolyDataMapper()
        maceMapper.SetInputConnection(apd.GetOutputPort())
        maceActor = vtk.vtkLODActor()
        maceActor.SetMapper(maceMapper)
        maceActor.VisibilityOn()

        # This portion of the code clips the mace with the vtkPlanes implicit function.
        # The clipped region is colored green.
        planes = vtk.vtkPlanes()
        clipper = vtk.vtkClipPolyData()
        clipper.SetInputConnection(apd.GetOutputPort())
        clipper.SetClipFunction(planes)
        clipper.InsideOutOn()
        selectMapper = vtk.vtkPolyDataMapper()
        selectMapper.SetInputConnection(clipper.GetOutputPort())
        selectActor = vtk.vtkLODActor()
        selectActor.SetMapper(selectMapper)
        selectActor.GetProperty().SetColor(0, 1, 0)
        selectActor.VisibilityOff()
        selectActor.SetScale(1.01, 1.01, 1.01)

        # Create the RenderWindow, Renderer and both Actors
        #
        ren = vtk.vtkRenderer()
        renWin = vtk.vtkRenderWindow()
        renWin.AddRenderer(ren)

        iRen = vtk.vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        #iren = vtk.vtkRenderWindowInteractor()
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
        boxRep = vtk.vtkBoxRepresentation()
        boxRep.SetPlaceFactor(0.75)
        boxRep.PlaceWidget(glyph.GetOutput().GetBounds())
        boxWidget = vtk.vtkBoxWidget2()
        boxWidget.SetInteractor(iRen)
        boxWidget.SetRepresentation(boxRep)
        boxWidget.AddObserver("EndInteractionEvent", SelectPolygons)
        boxWidget.SetPriority(1)

        # record events
        recorder = vtk.vtkInteractorEventRecorder()
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
        vtk.test.Testing.compareImage(iRen.GetRenderWindow(), vtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtk.test.Testing.interact()

if __name__ == "__main__":
     vtk.test.Testing.main([(TestInteractorEventRecorder, 'test')])
