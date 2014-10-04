"""Test swig-style wrapped pointers

Created on Oct 2, 2014 by David Gobbi
"""

import sys
import vtk
from vtk.test import Testing

class TestSwigPointer(Testing.vtkTest):
    def testVoidPointer(self):
        a = vtk.vtkIntArray()
        a.SetNumberOfTuples(1)
        a.SetValue(0, 1)
        b = vtk.vtkIntArray()
        b.SetNumberOfTuples(1)
        b.SetValue(0, 2)
        ptr = a.GetVoidPointer(0)
        # check the format _0123456789abcdef_p_void
        self.assertEqual(ptr[0:1], "_")
        self.assertEqual(ptr[-7:], "_p_void")
        address = int(ptr[1:-7], 16)
        # check that we can use the pointer
        b.SetVoidArray(ptr, 1, 1)
        self.assertEqual(b.GetValue(0), 1)
        a.SetValue(0, 10)
        self.assertEqual(b.GetValue(0), 10)

    def testObjectPointer(self):
        a = vtk.vtkInformation()
        ptr = a.__this__
        # check the format _0123456789abcdef_p_vtkInformation
        self.assertEqual(ptr[0:1], "_")
        self.assertEqual(ptr[-17:], "_p_vtkInformation")
        address = int(ptr[1:-17], 16)
        # create a VTK object from the swig pointer
        b = vtk.vtkObject(ptr)
        self.assertEqual(b.GetClassName(), a.GetClassName())
        self.assertEqual(a.__this__, b.__this__)
        self.assertEqual(a, b)

if __name__ == "__main__":
    Testing.main([(TestSwigPointer, 'test')])
