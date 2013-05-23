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

class StyleBaseSpike(object):

    class Colors(object):
        '''
            Provides some wrappers for using color names.
        '''
        def __init__(self):
            '''
                Define a single instance of the NamedColors class here.
            '''
            self.namedColors = vtk.vtkNamedColors()

        def GetRGBColor(self, colorName):
            '''
                Return the red, green and blue components for a
                color as doubles.
            '''
            rgb = [0.0, 0.0, 0.0] # black
            self.namedColors.GetColorRGB(colorName, rgb)
            return rgb

        def GetRGBAColor(self, colorName):
            '''
                Return the red, green, blue and alpha
                components for a color as doubles.
            '''
            rgba = [0.0, 0.0, 0.0, 1.0] # black
            self.namedColors.GetColor(colorName, rgba)
            return rgba

    def __init__(self, ren, renWin, iren):

        self.ren = ren
        self.renWin = renWin
        self.iren = iren

        colors = self.Colors()

        self.renWin.AddRenderer(self.ren)

        self.iren.SetRenderWindow(self.renWin)
        self.iren.SetDesiredUpdateRate(.00001)

        # Create a sphere source and actor

        sphere = vtk.vtkSphereSource()

        sphereMapper = vtk.vtkPolyDataMapper()
        sphereMapper.SetInputConnection(sphere.GetOutputPort())

        sphereActor = vtk.vtkLODActor()
        sphereActor.SetMapper(sphereMapper)

        sphereActor.GetProperty().SetDiffuseColor(colors.GetRGBColor('banana'))
        sphereActor.GetProperty().SetSpecular(.4)
        sphereActor.GetProperty().SetSpecularPower(20)

        # Create the spikes using a cone source and the sphere source

        cone = vtk.vtkConeSource()
        cone.SetResolution(20)

        glyph = vtk.vtkGlyph3D()
        glyph.SetInputConnection(sphere.GetOutputPort())
        glyph.SetSourceConnection(cone.GetOutputPort())
        glyph.SetVectorModeToUseNormal()
        glyph.SetScaleModeToScaleByVector()
        glyph.SetScaleFactor(0.25)

        spikeMapper = vtk.vtkPolyDataMapper()
        spikeMapper.SetInputConnection(glyph.GetOutputPort())

        spikeActor = vtk.vtkLODActor()
        spikeActor.SetMapper(spikeMapper)

        spikeActor.GetProperty().SetDiffuseColor(colors.GetRGBColor('tomato'))
        spikeActor.GetProperty().SetSpecular(.4)
        spikeActor.GetProperty().SetSpecularPower(20)

        # Add the actors to the renderer, set the background and size

        self.ren.AddActor(sphereActor)
        self.ren.AddActor(spikeActor)
        self.ren.SetBackground(0.1, 0.2, 0.4)

        self.renWin.SetSize(300, 300)

        # Render the image

        self.ren.ResetCamera()
        cam1 = ren.GetActiveCamera()
        cam1.Zoom(1.4)
        cam1.Azimuth(30)
        cam1.Elevation(30)

        self.renWin.Render()

class TestStyleBaseSpike(vtk.test.Testing.vtkTest):

    def testStyleBaseSpike(self):
        ren = vtk.vtkRenderer()
        renWin = vtk.vtkRenderWindow()
        iRen = vtk.vtkRenderWindowInteractor()

        styleBaseSpike = StyleBaseSpike(ren, renWin, iRen)

        # render and interact with data
        renWin.Render()

        img_file = "TestStyleBaseSpike.png"
        vtk.test.Testing.compareImage(iRen.GetRenderWindow(), vtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtk.test.Testing.interact()

if __name__ == "__main__":
     vtk.test.Testing.main([(TestStyleBaseSpike, 'test')])
