#!/usr/bin/env python
import vtk
import vtk.test.Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()
import sys

class TestOpenSlideReader(vtk.test.Testing.vtkTest):

    def testCanReadFile(self):
        reader = vtk.vtkOpenSlideReader()
        self.assertEqual(reader.CanReadFile(VTK_DATA_ROOT + "/Data/RectGrid2.vtk"), 0)

    def testCanNotReadFile(self):
        reader = vtk.vtkOpenSlideReader()
        self.assertEqual(reader.CanReadFile(VTK_DATA_ROOT + "/Data/Microscopy/small2.ndpi"), 2)

if __name__ == "__main__":
    vtk.test.Testing.main([(TestOpenSlideReader, 'test')])
