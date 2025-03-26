#!/usr/bin/env python
# -*- coding: utf-8 -*-



import sys
from vtkmodules.vtkRenderingCore import (
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

'''
  Prevent .pyc files from being created.
  Stops the vtk source being polluted
  by .pyc files.
'''
sys.dont_write_bytecode = True

import TestFixedPointRayCasterNearest

class TestFixedPointRayCasterNearestCropped(vtkmodules.test.Testing.vtkTest):

    def testFixedPointRayCasterNearestCropped(self):

        ren = vtkRenderer()
        renWin = vtkRenderWindow()
        iRen = vtkRenderWindowInteractor()

        tFPRCN = TestFixedPointRayCasterNearest.FixedPointRayCasterNearest(ren, renWin, iRen)
        volumeMapper = tFPRCN.GetVolumeMapper()

        for j in range(0, 5):
            for i in range(0, 5):
                volumeMapper[i][j].SetCroppingRegionPlanes(10, 20, 10, 20, 10, 20)
                volumeMapper[i][j].SetCroppingRegionFlags(253440)
                volumeMapper[i][j].CroppingOn()

        # render and interact with data

        renWin.Render()

        img_file = "TestFixedPointRayCasterNearestCropped.png"
        vtkmodules.test.Testing.compareImage(iRen.GetRenderWindow(), vtkmodules.test.Testing.getAbsImagePath(img_file), threshold=10)
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
     vtkmodules.test.Testing.main([(TestFixedPointRayCasterNearestCropped, 'test')])
