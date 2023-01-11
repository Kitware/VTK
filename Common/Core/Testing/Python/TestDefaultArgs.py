"""Test methods that use default parameter values.

Created on Feb 9, 2016 by David Gobbi
"""

import sys
from vtkmodules.vtkCommonCore import (
    VTK_UNSIGNED_CHAR,
    vtkIntArray,
)
from vtkmodules.vtkCommonDataModel import vtkImageData
from vtkmodules.vtkImagingCore import (
    vtkImagePointDataIterator,
    vtkImagePointIterator,
)
from vtkmodules.test import Testing

class TestDefaultArgs(Testing.vtkTest):
    def testDefaultInt(self):
        """Simple test of an integer arg with default value."""
        image = vtkImageData()
        image.SetExtent(0,9,0,9,0,9)
        image.AllocateScalars(VTK_UNSIGNED_CHAR, 1)
        ipi = vtkImagePointIterator()
        # call this method with the threadId parameter set to 0
        ipi.Initialize(image, (0,9,0,9,0,9), None, None, 0)
        # call this method without the threadId parameter
        ipi.Initialize(image, (0,9,0,9,0,9), None, None)

    def testDefaultObjectPointer(self):
        """Test a vtkObject pointer arg with default value of 0."""
        image = vtkImageData()
        image.SetExtent(0,9,0,9,0,9)
        image.AllocateScalars(VTK_UNSIGNED_CHAR, 1)
        ipi = vtkImagePointIterator()
        # call this method with the stencil parameter set to None
        ipi.Initialize(image, (0,9,0,9,0,9), None)
        # call this method without the stencil parameter
        ipi.Initialize(image, (0,9,0,9,0,9))

    def testDefaultArray(self):
        """Test an array arg with default value of 0."""
        image = vtkImageData()
        image.SetExtent(0,9,0,9,0,9)
        image.AllocateScalars(VTK_UNSIGNED_CHAR, 1)
        ipi = vtkImagePointIterator()
        # call this method with the parameter set
        ipi.Initialize(image, (0,9,0,9,0,9))
        # call this method without extent parameter
        ipi.Initialize(image)

    def testDefaultPointer(self):
        """Test a POD pointer arg with default value of 0."""
        a = vtkIntArray()
        a.SetNumberOfComponents(3)
        # pass an int pointer arg, expect something back
        inc = [0]
        vtkImagePointDataIterator.GetVoidPointer(a, 0, inc)
        self.assertEqual(inc, [3])
        # do not pass the pointer arg, default value 0 is passed
        vtkImagePointDataIterator.GetVoidPointer(a, 0)

if __name__ == "__main__":
    Testing.main([(TestDefaultArgs, 'test')])
