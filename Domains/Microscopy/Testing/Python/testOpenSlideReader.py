#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()
import sys

class TestOpenSlideReader(Testing.vtkTest):

    def testCanReadFile(self):
        reader = vtk.vtkOpenSlideReader()
        self.assertEqual(reader.CanReadFile(VTK_DATA_ROOT + "/Data/RectGrid2.vtk"), 0)

    def testCanNotReadFile(self):
        reader = vtk.vtkOpenSlideReader()
        self.assertEqual(reader.CanReadFile(VTK_DATA_ROOT + "/Data/Microscopy/small2.ndpi"), 2)

if __name__ == "__main__":
    Testing.main([(TestOpenSlideReader, 'test')])
