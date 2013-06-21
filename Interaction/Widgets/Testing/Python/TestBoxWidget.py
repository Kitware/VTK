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

import vtk
import vtk.test.Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class TestBoxWidget(vtk.test.Testing.vtkTest):

    def testBoxWidget(self):

        # Demonstrate how to use the vtkBoxWidget.
        # This script uses a 3D box widget to define a "clipping box" to clip some
        # simple geometry (a mace). Make sure that you hit the "W" key to activate the widget.

        # create a sphere source
        #
        sphere = vtk.vtkSphereSource()
        cone = vtk.vtkConeSource()
        glyph = vtk.vtkGlyph3D()
        glyph.SetInputConnection(sphere.GetOutputPort())
        glyph.SetSourceConnection(cone.GetOutputPort())
        glyph.SetVectorModeToUseNormal()
        glyph.SetScaleModeToScaleByVector()
        glyph.SetScaleFactor(0.25)
        apd = vtk.vtkAppendPolyData()
        apd.AddInputConnection(glyph.GetOutputPort())
        apd.AddInputConnection(sphere.GetOutputPort())
        maceMapper = vtk.vtkPolyDataMapper()
        maceMapper.SetInputConnection(apd.GetOutputPort())
        maceActor = vtk.vtkLODActor()
        maceActor.SetMapper(maceMapper)
        maceActor.VisibilityOn()

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
        boxWidget = vtk.vtkBoxWidget()
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
        vtk.test.Testing.compareImage(iRen.GetRenderWindow(), vtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtk.test.Testing.interact()

if __name__ == "__main__":
     vtk.test.Testing.main([(TestBoxWidget, 'test')])
