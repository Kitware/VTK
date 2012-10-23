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
# vtkpython TestAllShrinks.py  -D $VTK_DATA_ROOT \
# -B $VTK_DATA_ROOT/Baseline/Imaging

import vtk
import vtk.test.Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class TestAllShrinks(vtk.test.Testing.vtkTest):

    def testAllShrinks(self):


        prefix = VTK_DATA_ROOT + "/Data/headsq/quarter"


        renWin = vtk.vtkRenderWindow()

        # Image pipeline
        reader = vtk.vtkImageReader()
        reader.SetDataExtent(0, 63, 0, 63, 1, 93)
        reader.SetFilePrefix(prefix)
        reader.SetDataByteOrderToLittleEndian()
        reader.SetDataMask(0x7fff)

        factor = 4
        magFactor = 8

        ops = ["Minimum", "Maximum", "Mean", "Median", "NoOp"]

        shrink = dict()
        mag = dict()
        mapper = dict()
        actor = dict()
        imager = dict()

        for operator in ops:
            shrink.update({operator:vtk.vtkImageShrink3D()})
            shrink[operator].SetMean(0)
            if operator != "NoOp":
             eval('shrink[operator].' + operator + 'On()')
            shrink[operator].SetShrinkFactors(factor, factor, factor)
            shrink[operator].SetInputConnection(reader.GetOutputPort())
            mag.update({operator:vtk.vtkImageMagnify()})
            mag[operator].SetMagnificationFactors(magFactor, magFactor, magFactor)
            mag[operator].InterpolateOff()
            mag[operator].SetInputConnection(shrink[operator].GetOutputPort())
            mapper.update({operator:vtk.vtkImageMapper()})
            mapper[operator].SetInputConnection(mag[operator].GetOutputPort())
            mapper[operator].SetColorWindow(2000)
            mapper[operator].SetColorLevel(1000)
            mapper[operator].SetZSlice(45)
            actor.update({operator:vtk.vtkActor2D()})
            actor[operator].SetMapper(mapper[operator])
            imager.update({operator:vtk.vtkRenderer()})
            imager[operator].AddActor2D(actor[operator])
            renWin.AddRenderer(imager[operator])

        shrink["Minimum"].Update
        shrink["Maximum"].Update
        shrink["Mean"].Update
        shrink["Median"].Update

        imager["Minimum"].SetViewport(0, 0, .5, .33)
        imager["Maximum"].SetViewport(0, .33, .5, .667)
        imager["Mean"].SetViewport(.5, 0, 1, .33)
        imager["Median"].SetViewport(.5, .33, 1, .667)
        imager["NoOp"].SetViewport(0, .667, 1, 1)

        renWin.SetSize(256, 384)

        # render and interact with data

        iRen = vtk.vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "TestAllShrinks.png"
        vtk.test.Testing.compareImage(iRen.GetRenderWindow(), vtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtk.test.Testing.interact()

if __name__ == "__main__":
     vtk.test.Testing.main([(TestAllShrinks, 'test')])
