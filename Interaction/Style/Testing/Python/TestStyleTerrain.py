#!/usr/bin/env python
# -*- coding: utf-8 -*-



import sys
from vtkmodules.vtkInteractionStyle import vtkInteractorStyleTerrain
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

# Load base (spike and test)
import TestStyleBaseSpike
import TestStyleBase

class TestStyleTerrain(vtkmodules.test.Testing.vtkTest):

    def testStyleTerrain(self):

        ren = vtkRenderer()
        renWin = vtkRenderWindow()
        iRen = vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);

        testStyleBaseSpike = TestStyleBaseSpike.StyleBaseSpike(ren, renWin, iRen)

        # Set interactor style
        inStyle = vtkInteractorStyleTerrain()
        iRen.SetInteractorStyle(inStyle)

        # Test style
        testStyleBase = TestStyleBase.TestStyleBase(ren)
        testStyleBase.test_style(inStyle)

        # render and interact with data

        img_file = "TestStyleTerrain.png"
        vtkmodules.test.Testing.compareImage(iRen.GetRenderWindow(), vtkmodules.test.Testing.getAbsImagePath(img_file))
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
     vtkmodules.test.Testing.main([(TestStyleTerrain, 'test')])
