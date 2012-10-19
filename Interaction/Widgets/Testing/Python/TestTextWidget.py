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

# Run this test like so:
# vtkpython TestTextWidget.py  -D $VTK_DATA_ROOT \
# -B $VTK_DATA_ROOT/Baseline/Widgets

import vtk
import vtk.test.Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class TestTextWidget(vtk.test.Testing.vtkTest):

    def testTextWidget(self):

        # Create fake data
        #
        ss = vtk.vtkSphereSource()
        mapper = vtk.vtkPolyDataMapper()
        mapper.SetInputConnection(ss.GetOutputPort())
        actor = vtk.vtkActor()
        actor.SetMapper(mapper)

        # Create the RenderWindow, Renderer and both Actors
        #
        ren = vtk.vtkRenderer()
        renWin = vtk.vtkRenderWindow()
        renWin.AddRenderer(ren)
        iRen = vtk.vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin)

        ren.AddActor(actor)
        ren.SetBackground(0.1, 0.2, 0.4)
        renWin.SetSize(300, 300)

        iRen.Initialize()
        renWin.Render()

        widget = vtk.vtkTextWidget()
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
        vtk.test.Testing.compareImage(iRen.GetRenderWindow(), vtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtk.test.Testing.interact()

if __name__ == "__main__":
     vtk.test.Testing.main([(TestTextWidget, 'test')])
