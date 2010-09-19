"""Test passing arguments of various types

Created on Sept 19, 2010 by David Gobbi
"""

import sys
import vtk
from vtk.test import Testing

arrays = [
  vtk.vtkFloatArray,
  vtk.vtkDoubleArray,
  vtk.vtkSignedCharArray,
  vtk.vtkShortArray,
  vtk.vtkIntArray,
  vtk.vtkLongArray,
  vtk.vtkTypeInt64Array,
  vtk.vtkIdTypeArray,
]

unsignedArrays = [
  vtk.vtkUnsignedCharArray,
  vtk.vtkUnsignedShortArray,
  vtk.vtkUnsignedIntArray,
  vtk.vtkUnsignedLongArray,
  vtk.vtkTypeUInt64Array,
]

class TestArrayArguments(Testing.vtkTest):
    def testArrayArguments(self):
        for arrayClass in arrays:
            a = arrayClass()
            a.SetNumberOfComponents(3)
            a.SetNumberOfTuples(1)
            ti = [0, a.GetDataTypeValueMin(), a.GetDataTypeValueMax()]
            to = [0, 0, 0]
            a.SetTupleValue(0, ti)
            a.GetTupleValue(0, to);
            self.assertEqual(ti, to)
            d1 = a.GetTuple(0)
            d2 = map(float, ti)
            self.assertEqual(map(round, d1), map(round, d2))

    def testUnsignedArrayArguments(self):
        for arrayClass in unsignedArrays:
            a = arrayClass()
            a.SetNumberOfComponents(3)
            a.SetNumberOfTuples(1)
            ti = [0, a.GetDataTypeValueMin(), a.GetDataTypeValueMax()]
            to = [0, 0, 0]
            a.SetTupleValue(0, ti)
            a.GetTupleValue(0, to);
            self.assertEqual(ti, to)
            d1 = a.GetTuple(0)
            d2 = map(float, ti)
            self.assertEqual(map(round, d1), map(round, d2))

    def testCharArrayArguments(self):
        a = vtk.vtkCharArray()
        a.SetNumberOfComponents(3)
        a.SetNumberOfTuples(1)
        ti = "opn"
        a.SetTupleValue(0, ti)
        # python strings are immutable, so this should NOT work
        #to = "***"
        #a.GetTupleValue(0, to);
        d1 = list(a.GetTuple(0))
        d2 = map(float, map(ord, ti))
        self.assertEqual(d1, d2)

    def testBitArrayArguments(self):
        a = vtk.vtkBitArray()
        a.SetNumberOfComponents(2)
        a.SetNumberOfTuples(1)
        ti = [0,1]
        to = [0,0]
        a.SetTuple(0, ti)
        a.GetTuple(0, to);
        self.assertEqual(ti, map(int,to))

    def testNDimArrayArguments(self):
        a = [[0,0,0],[0,0,0],[0,0,0]]
        vtk.vtkMath.Identity3x3(a)
        x = [0.5, 0.2, 0.1]
        y = [0.0, 0.0, 0.0]
        vtk.vtkMath.Multiply3x3(a, x, y)
        self.assertEqual(x, y)

if __name__ == "__main__":
    Testing.main([(TestArrayArguments, 'test')])
