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
# vtkpython TestFixedPointRayCasterLinearCropped.py  -D $VTK_DATA_ROOT \
# -B $VTK_DATA_ROOT/Baseline/VolumeRendering

import sys
import vtk
import vtk.test.Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

'''
  Prevent .pyc files from being created.
  Stops the vtk source being polluted
  by .pyc files.
'''
sys.dont_write_bytecode = True

import TestFixedPointRayCasterNearest

class TestFixedPointRayCasterLinearCropped(vtk.test.Testing.vtkTest):

    def testFixedPointRayCasterLinearCropped(self):

        ren = vtk.vtkRenderer()
        renWin = vtk.vtkRenderWindow()
        iRen = vtk.vtkRenderWindowInteractor()

        tFPRCN = TestFixedPointRayCasterNearest.FixedPointRayCasterNearest(ren, renWin, iRen)
        volumeProperty = tFPRCN.GetVolumeProperty()
        volumeMapper = tFPRCN.GetVolumeMapper()

        for j in range(0, 5):
            for i in range(0, 5):
                volumeMapper[i][j].SetCroppingRegionPlanes(10, 20, 10, 20, 10, 20)
                volumeMapper[i][j].SetCroppingRegionFlags(253440)
                volumeMapper[i][j].CroppingOn()

                volumeProperty[i][j].SetInterpolationTypeToLinear()

        # render and interact with data

        renWin.Render()

        img_file = "TestFixedPointRayCasterLinearCropped.png"
        vtk.test.Testing.compareImage(iRen.GetRenderWindow(), vtk.test.Testing.getAbsImagePath(img_file), threshold=10)
        vtk.test.Testing.interact()

if __name__ == "__main__":
     vtk.test.Testing.main([(TestFixedPointRayCasterLinearCropped, 'test')])
