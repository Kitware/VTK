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

import StringIO
import sys
import vtk
import vtk.test.Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class MassProperties(vtk.test.Testing.vtkTest):

    def testMassProperties(self):

        # Create the RenderWindow, Renderer and both Actors
        #
        ren = vtk.vtkRenderer()
        renWin = vtk.vtkRenderWindow()
        renWin.AddRenderer(ren)

        cone = vtk.vtkConeSource()
        cone.SetResolution(50)

        sphere = vtk.vtkSphereSource()
        sphere.SetPhiResolution(50)
        sphere.SetThetaResolution(50)

        cube = vtk.vtkCubeSource()
        cube.SetXLength(1)
        cube.SetYLength(1)
        cube.SetZLength(1)

        sphereMapper = vtk.vtkPolyDataMapper()
        sphereMapper.SetInputConnection(sphere.GetOutputPort())
        sphereMapper.GlobalImmediateModeRenderingOn()

        sphereActor = vtk.vtkActor()
        sphereActor.SetMapper(sphereMapper)
        sphereActor.GetProperty().SetDiffuseColor(1, .2, .4)
        coneMapper = vtk.vtkPolyDataMapper()
        coneMapper.SetInputConnection(cone.GetOutputPort())
        coneMapper.GlobalImmediateModeRenderingOn()

        coneActor = vtk.vtkActor()
        coneActor.SetMapper(coneMapper)
        coneActor.GetProperty().SetDiffuseColor(.2, .4, 1)

        cubeMapper = vtk.vtkPolyDataMapper()
        cubeMapper.SetInputConnection(cube.GetOutputPort())
        cubeMapper.GlobalImmediateModeRenderingOn()

        cubeActor = vtk.vtkActor()
        cubeActor.SetMapper(cubeMapper)
        cubeActor.GetProperty().SetDiffuseColor(.2, 1, .4)

        #Add the actors to the renderer, set the background and size
        #
        sphereActor.SetPosition(-5, 0, 0)
        ren.AddActor(sphereActor)
        coneActor.SetPosition(0, 0, 0)
        ren.AddActor(coneActor)
        coneActor.SetPosition(5, 0, 0)
        ren.AddActor(cubeActor)

        tf = dict()
        mp = dict()
        vt = dict()
        pdm = dict()
        ta = dict()

        def MakeText(primitive):

            tf.update({primitive: vtk.vtkTriangleFilter()})
            tf[primitive].SetInputConnection(primitive.GetOutputPort())

            mp.update({primitive: vtk.vtkMassProperties()})
            mp[primitive].SetInputConnection(tf[primitive].GetOutputPort())

            # here we capture stdout and write it to a variable for processing.
            summary = StringIO.StringIO()
            # save the original stdout
            old_stdout = sys.stdout
            sys.stdout = summary

            print mp[primitive]
            summary = summary.getvalue()

            startSum = summary.find("  VolumeX")
            endSum = len(summary)
            print summary[startSum:]
            # Restore stdout
            sys.stdout = old_stdout

            vt.update({primitive: vtk.vtkVectorText()})
            vt[primitive].SetText(summary[startSum:])

            pdm.update({primitive: vtk.vtkPolyDataMapper()})
            pdm[primitive].SetInputConnection(vt[primitive].GetOutputPort())

            ta.update({primitive: vtk.vtkActor()})
            ta[primitive].SetMapper(pdm[primitive])
            ta[primitive].SetScale(.2, .2, .2)
            return ta[primitive]


        ren.AddActor(MakeText(sphere))
        ren.AddActor(MakeText(cube))
        ren.AddActor(MakeText(cone))

        ta[sphere].SetPosition(sphereActor.GetPosition())
        ta[sphere].AddPosition(-2, -1, 0)
        ta[cube].SetPosition(cubeActor.GetPosition())
        ta[cube].AddPosition(-2, -1, 0)
        ta[cone].SetPosition(coneActor.GetPosition())
        ta[cone].AddPosition(-2, -1, 0)

        ren.SetBackground(0.1, 0.2, 0.4)
        renWin.SetSize(786, 256)

        # render the image
        #
        ren.ResetCamera()
        cam1 = ren.GetActiveCamera()
        cam1.Dolly(3)
        ren.ResetCameraClippingRange()

        # render and interact with data

        iRen = vtk.vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "MassProperties.png"
        vtk.test.Testing.compareImage(iRen.GetRenderWindow(), vtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtk.test.Testing.interact()

if __name__ == "__main__":
     vtk.test.Testing.main([(MassProperties, 'test')])
