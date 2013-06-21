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

class TestAllMaskBits(vtk.test.Testing.vtkTest):

    def testAllMaskBits(self):

        # This script calculates the luminance of an image

        renWin = vtk.vtkRenderWindow()


        # Image pipeline

        image1 = vtk.vtkTIFFReader()
        image1.SetFileName(VTK_DATA_ROOT + "/Data/beach.tif")

        # "beach.tif" image contains ORIENTATION tag which is
        # ORIENTATION_TOPLEFT (row 0 top, col 0 lhs) type. The TIFF
        # reader parses this tag and sets the internal TIFF image
        # orientation accordingly.  To overwrite this orientation with a vtk
        # convention of ORIENTATION_BOTLEFT (row 0 bottom, col 0 lhs ), invoke
        # SetOrientationType method with parameter value of 4.
        image1.SetOrientationType(4)

        shrink = vtk.vtkImageShrink3D()
        shrink.SetInputConnection(image1.GetOutputPort())
        shrink.SetShrinkFactors(2, 2, 1)

        operators = ["ByPass", "And", "Nand", "Xor", "Or", "Nor"]

        operator = dict()
        mapper = dict()
        actor = dict()
        imager = dict()

        for idx, op in enumerate(operators):
            if op != "ByPass":
                operator.update({idx: vtk.vtkImageMaskBits()})
                operator[idx].SetInputConnection(shrink.GetOutputPort())
                eval('operator[' + str(idx) + '].SetOperationTo' + op + '()')
                operator[idx].SetMasks(255, 255, 0)

            mapper.update({idx: vtk.vtkImageMapper()})
            if op != "ByPass":
                mapper[idx].SetInputConnection(operator[idx].GetOutputPort())
            else:
                mapper[idx].SetInputConnection(shrink.GetOutputPort())
            mapper[idx].SetColorWindow(255)
            mapper[idx].SetColorLevel(127.5)

            actor.update({idx: vtk.vtkActor2D()})
            actor[idx].SetMapper(mapper[idx])

            imager.update({idx: vtk.vtkRenderer()})
            imager[idx].AddActor2D(actor[idx])

            renWin.AddRenderer(imager[idx])


        column = 0
        row = 0
        deltaX = 1.0 / 3.0
        deltaY = 1.0 / 2.0

        for idx in range(len(operators)):
            imager[idx].SetViewport(column * deltaX, row * deltaY, (column + 1) * deltaX, (row + 1) * deltaY)
            column += 1
            if column > 2:
                column = 0
                row += 1

        renWin.SetSize(384, 256)

        # render and interact with data

        iRen = vtk.vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "TestAllMaskBits.png"
        vtk.test.Testing.compareImage(iRen.GetRenderWindow(), vtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtk.test.Testing.interact()

if __name__ == "__main__":
     vtk.test.Testing.main([(TestAllMaskBits, 'test')])
