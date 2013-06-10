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

class reconstructSurface(vtk.test.Testing.vtkTest):

    def testReconstructSurface(self):

        # Read some points. Use a programmable filter to read them.
        #
        pointSource = vtk.vtkProgrammableSource()

        def readPoints():

            fp = open(VTK_DATA_ROOT + "/Data/cactus.3337.pts", "r")

            points = vtk.vtkPoints()
            while True:
                line = fp.readline().split()
                if len(line) == 0:
                    break
                if line[0] == "p":
                    points.InsertNextPoint(float(line[1]), float(line[2]), float(line[3]))
            pointSource.GetPolyDataOutput().SetPoints(points)

        pointSource.SetExecuteMethod(readPoints)

        # Construct the surface and create isosurface
        #
        surf = vtk.vtkSurfaceReconstructionFilter()
        surf.SetInputConnection(pointSource.GetOutputPort())

        cf = vtk.vtkContourFilter()
        cf.SetInputConnection(surf.GetOutputPort())
        cf.SetValue(0, 0.0)

        reverse = vtk.vtkReverseSense()
        reverse.SetInputConnection(cf.GetOutputPort())
        reverse.ReverseCellsOn()
        reverse.ReverseNormalsOn()

        map = vtk.vtkPolyDataMapper()
        map.SetInputConnection(reverse.GetOutputPort())
        map.ScalarVisibilityOff()

        surfaceActor = vtk.vtkActor()
        surfaceActor.SetMapper(map)
        surfaceActor.GetProperty().SetDiffuseColor(1.0000, 0.3882, 0.2784)
        surfaceActor.GetProperty().SetSpecularColor(1, 1, 1)
        surfaceActor.GetProperty().SetSpecular(.4)
        surfaceActor.GetProperty().SetSpecularPower(50)

        # Create the RenderWindow, Renderer and both Actors
        #
        ren = vtk.vtkRenderer()
        renWin = vtk.vtkRenderWindow()
        renWin.AddRenderer(ren)

        # Add the actors to the renderer, set the background and size
        #
        ren.AddActor(surfaceActor)
        ren.SetBackground(1, 1, 1)
        renWin.SetSize(300, 300)
        ren.GetActiveCamera().SetFocalPoint(0, 0, 0)
        ren.GetActiveCamera().SetPosition(1, 0, 0)
        ren.GetActiveCamera().SetViewUp(0, 0, 1)
        ren.ResetCamera()
        ren.GetActiveCamera().Azimuth(20)
        ren.GetActiveCamera().Elevation(30)
        ren.GetActiveCamera().Dolly(1.2)
        ren.ResetCameraClippingRange()

        # render and interact with data

        iRen = vtk.vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "reconstructSurface.png"
        vtk.test.Testing.compareImage(iRen.GetRenderWindow(), vtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtk.test.Testing.interact()

if __name__ == "__main__":
     vtk.test.Testing.main([(reconstructSurface, 'test')])
