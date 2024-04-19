#!/usr/bin/env python
from vtkmodules.vtkDomainsMicroscopy import vtkOpenSlideReader
import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()
import sys

class TestOpenSlideReader(vtkmodules.test.Testing.vtkTest):

    def testCanReadFile(self):
        reader = vtkOpenSlideReader()
        self.assertEqual(reader.CanReadFile(VTK_DATA_ROOT + "/Data/RectGrid2.vtk"), 0)

    def testCanNotReadFile(self):
        reader = vtkOpenSlideReader()
        self.assertEqual(reader.CanReadFile(VTK_DATA_ROOT + "/Data/Microscopy/small2.ndpi"), 2)

if __name__ == "__main__":
    vtkmodules.test.Testing.main([(TestOpenSlideReader, 'test')])
