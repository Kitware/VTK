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

class TestPlatonicSolids(vtk.test.Testing.vtkTest):

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

    def testPlatonicSolids(self):

        # Create five instances of vtkPlatonicSolidSource
        # corresponding to each of the five Platonic solids.
        #
        tet = vtk.vtkPlatonicSolidSource()
        tet.SetSolidTypeToTetrahedron()
        tetMapper = vtk.vtkPolyDataMapper()
        tetMapper.SetInputConnection(tet.GetOutputPort())
        tetActor = vtk.vtkActor()
        tetActor.SetMapper(tetMapper)

        cube = vtk.vtkPlatonicSolidSource()
        cube.SetSolidTypeToCube()
        cubeMapper = vtk.vtkPolyDataMapper()
        cubeMapper.SetInputConnection(cube.GetOutputPort())
        cubeActor = vtk.vtkActor()
        cubeActor.SetMapper(cubeMapper)
        cubeActor.AddPosition(2.0, 0, 0)

        oct = vtk.vtkPlatonicSolidSource()
        oct.SetSolidTypeToOctahedron()
        octMapper = vtk.vtkPolyDataMapper()
        octMapper.SetInputConnection(oct.GetOutputPort())
        octActor = vtk.vtkActor()
        octActor.SetMapper(octMapper)
        octActor.AddPosition(4.0, 0, 0)

        icosa = vtk.vtkPlatonicSolidSource()
        icosa.SetSolidTypeToIcosahedron()
        icosaMapper = vtk.vtkPolyDataMapper()
        icosaMapper.SetInputConnection(icosa.GetOutputPort())
        icosaActor = vtk.vtkActor()
        icosaActor.SetMapper(icosaMapper)
        icosaActor.AddPosition(6.0, 0, 0)

        dode = vtk.vtkPlatonicSolidSource()
        dode.SetSolidTypeToDodecahedron()
        dodeMapper = vtk.vtkPolyDataMapper()
        dodeMapper.SetInputConnection(dode.GetOutputPort())
        dodeActor = vtk.vtkActor()
        dodeActor.SetMapper(dodeMapper)
        dodeActor.AddPosition(8.0, 0, 0)

        # Create rendering stuff
        #
        ren = vtk.vtkRenderer()
        renWin = vtk.vtkRenderWindow()
        renWin.AddRenderer(ren)

        # Add the actors to the renderer, set the background and size
        #
        ren.AddActor(tetActor)
        ren.AddActor(cubeActor)
        ren.AddActor(octActor)
        ren.AddActor(icosaActor)
        ren.AddActor(dodeActor)

        colors = self.Colors()

        # Create a lookup table with colors for each face
        #
        math = vtk.vtkMath()
        lut = vtk.vtkLookupTable()
        lut.SetNumberOfColors(20)
        lut.Build()
        lut.SetTableValue(0, colors.GetRGBAColor("red"))
        lut.SetTableValue(1, colors.GetRGBAColor("lime"))
        lut.SetTableValue(2, colors.GetRGBAColor("yellow"))
        lut.SetTableValue(3, colors.GetRGBAColor("blue"))
        lut.SetTableValue(4, colors.GetRGBAColor("magenta"))
        lut.SetTableValue(5, colors.GetRGBAColor("cyan"))
        lut.SetTableValue(6, colors.GetRGBAColor("spring_green"))
        lut.SetTableValue(7, colors.GetRGBAColor("lavender"))
        lut.SetTableValue(8, colors.GetRGBAColor("mint_cream"))
        lut.SetTableValue(9, colors.GetRGBAColor("violet"))
        lut.SetTableValue(10, colors.GetRGBAColor("ivory_black"))
        lut.SetTableValue(11, colors.GetRGBAColor("coral"))
        lut.SetTableValue(12, colors.GetRGBAColor("pink"))
        lut.SetTableValue(13, colors.GetRGBAColor("salmon"))
        lut.SetTableValue(14, colors.GetRGBAColor("sepia"))
        lut.SetTableValue(15, colors.GetRGBAColor("carrot"))
        lut.SetTableValue(16, colors.GetRGBAColor("gold"))
        lut.SetTableValue(17, colors.GetRGBAColor("forest_green"))
        lut.SetTableValue(18, colors.GetRGBAColor("turquoise"))
        lut.SetTableValue(19, colors.GetRGBAColor("plum"))

        lut.SetTableRange(0, 19)
        tetMapper.SetLookupTable(lut)
        tetMapper.SetScalarRange(0, 19)
        cubeMapper.SetLookupTable(lut)
        cubeMapper.SetScalarRange(0, 19)
        octMapper.SetLookupTable(lut)
        octMapper.SetScalarRange(0, 19)
        icosaMapper.SetLookupTable(lut)
        icosaMapper.SetScalarRange(0, 19)
        dodeMapper.SetLookupTable(lut)
        dodeMapper.SetScalarRange(0, 19)

        cam = ren.GetActiveCamera()
        cam.SetPosition(3.89696, 7.20771, 1.44123)
        cam.SetFocalPoint(3.96132, 0, 0)
        cam.SetViewUp(-0.0079335, 0.196002, -0.980571)
        cam.SetClippingRange(5.42814, 9.78848)

        ren.SetBackground(colors.GetRGBColor("black"))
        renWin.SetSize(400, 150)

        # render and interact with data

        iRen = vtk.vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "TestPlatonicSolids.png"
        vtk.test.Testing.compareImage(iRen.GetRenderWindow(), vtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtk.test.Testing.interact()

if __name__ == "__main__":
     vtk.test.Testing.main([(TestPlatonicSolids, 'test')])
