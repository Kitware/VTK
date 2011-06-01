"""Test template support in VTK-Python

VTK-python decides which template specializations
to wrap according to which ones are used in typedefs
and which ones appear as superclasses of other classes.
In addition, the wrappers are hard-coded to wrap the
vtkDenseArray and vtkSparseArray classes over a broad
range of types.

Created on May 29, 2011 by David Gobbi
"""

import sys
import exceptions
import vtk
from vtk.test import Testing

arrayTypes = ['char', 'int8', 'uint8', 'int16', 'uint16',
              'int32', 'uint32', int, 'uint', 'int64', 'uint64',
              'float32', float, str, 'unicode', vtk.vtkVariant]

arrayCodes = ['c', 'b', 'B', 'h', 'H',
              'i', 'I', 'l', 'L', 'q', 'Q',
              'f', 'd']

class TestTemplates(Testing.vtkTest):

    def testDenseArray(self):
        """Test vtkDenseArray template"""
        for t in (arrayTypes + arrayCodes):
            a = vtk.vtkDenseArray[t]()
            a.Resize(1)
            i = vtk.vtkArrayCoordinates(0)
            if t in ['bool', '?']:
                value = 1
                a.SetValue(i, value)
                result = a.GetValue(i)
                self.assertEqual(value, result)
            elif t in ['float32', 'float64', 'float', 'f', 'd']:
                value = 3.125
                a.SetValue(i, value)
                result = a.GetValue(i)
                self.assertEqual(value, result)
            elif t in ['char', 'c']:
                value = 'c'
                a.SetValue(i, value)
                result = a.GetValue(i)
                self.assertEqual(value, result)
            elif t in [str, 'str', 'unicode']:
                value = unicode("hello")
                a.SetValue(i, value)
                result = a.GetValue(i)
                self.assertEqual(value, result)
            elif t in ['vtkVariant', vtk.vtkVariant]:
                value = vtk.vtkVariant("world")
                a.SetValue(i, value)
                result = a.GetValue(i)
                self.assertEqual(value, result)
            else:
                value = 12
                a.SetValue(i, value)
                result = a.GetValue(i)
                self.assertEqual(value, result)

    def testSparseArray(self):
        """Test vtkSparseArray template"""
        for t in (arrayTypes + arrayCodes):
            a = vtk.vtkSparseArray[t]()
            a.Resize(1)
            i = vtk.vtkArrayCoordinates(0)
            if t in ['bool', '?']:
                value = 0
                a.SetValue(i, value)
                result = a.GetValue(i)
                self.assertEqual(value, result)
            elif t in ['float32', 'float64', 'float', 'f', 'd']:
                value = 3.125
                a.SetValue(i, value)
                result = a.GetValue(i)
                self.assertEqual(value, result)
            elif t in ['char', 'c']:
                value = 'c'
                a.SetValue(i, value)
                result = a.GetValue(i)
                self.assertEqual(value, result)
            elif t in [str, 'str', 'unicode']:
                value = unicode("hello")
                a.SetValue(i, value)
                result = a.GetValue(i)
                self.assertEqual(value, result)
            elif t in ['vtkVariant', vtk.vtkVariant]:
                value = vtk.vtkVariant("world")
                a.SetValue(i, value)
                result = a.GetValue(i)
                self.assertEqual(value, result)
            else:
                value = 12
                a.SetValue(i, value)
                result = a.GetValue(i)
                self.assertEqual(value, result)

    def testArray(self):
        """Test array CreateArray"""
        o = vtk.vtkArray.CreateArray(vtk.vtkArray.DENSE, vtk.VTK_DOUBLE)
        self.assertEqual(o.__class__, vtk.vtkDenseArray[float])

    def testVector(self):
        """Test vector templates"""
        # make sure Rect inherits operators
        r = vtk.vtkRectf(0, 0, 2, 2)
        self.assertEqual(r[2], 2.0)
        c = vtk.vtkColor4ub()
        self.assertEqual(list(c), [0, 0, 0, 255])
        e = vtk.vtkVector['float32', 3]([0.0, 1.0, 2.0])
        self.assertEqual(list(e), [0.0, 1.0, 2.0])
        i = vtk.vtkVector3['i']()
        self.assertEqual(list(i), [0, 0, 0])

if __name__ == "__main__":
    Testing.main([(TestTemplates, 'test')])
