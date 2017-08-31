"""Test the use of preconditions with VTK_EXPECTS()
"""

import sys
import vtk
from vtk.test import Testing

class TestExpects(Testing.vtkTest):
    def testPoints(self):
        """Test the index limits for vtkPoints
        """
        points = vtk.vtkPoints()
        p = (1.0, 2.0, 3.0)
        points.InsertNextPoint(p)
        self.assertEqual(points.GetPoint(0), p)
        with self.assertRaises(ValueError):
            points.GetPoint(-1)
        with self.assertRaises(ValueError):
            points.GetPoint(1)
        with self.assertRaises(ValueError):
            points.SetPoint(-1, p)
        with self.assertRaises(ValueError):
            points.SetPoint(1, p)

    def testArray(self):
        """Test values, tuples, components of arrays
        """
        array = vtk.vtkDoubleArray()
        array.SetNumberOfComponents(2)
        t = (2.0, 10.0)
        array.InsertNextTuple(t)
        array.InsertNextTuple(t)
        array.InsertNextTuple(t)
        self.assertEqual(array.GetTuple(0), t)
        self.assertEqual(array.GetTuple(2), t)
        with self.assertRaises(ValueError):
            array.GetTuple(-1)
        with self.assertRaises(ValueError):
            array.GetTuple(3)
        with self.assertRaises(ValueError):
            array.SetTuple(-1, t)
        with self.assertRaises(ValueError):
            array.SetTuple(3, t)
        self.assertEqual(array.GetValue(0), 2.0)
        self.assertEqual(array.GetValue(5), 10.0)
        with self.assertRaises(ValueError):
            array.GetValue(-1)
        with self.assertRaises(ValueError):
            array.GetValue(6)
        with self.assertRaises(ValueError):
            array.SetValue(-1, 2.0)
        with self.assertRaises(ValueError):
            array.SetValue(6, 10.0)
        self.assertEqual(array.GetComponent(0, 1), 10.0)
        with self.assertRaises(ValueError):
            array.GetComponent(0, -1)
        with self.assertRaises(ValueError):
            array.GetComponent(0, 2)
        with self.assertRaises(ValueError):
            array.GetComponent(-1, 0)
        with self.assertRaises(ValueError):
            array.GetComponent(3, 1)
        with self.assertRaises(ValueError):
            array.SetComponent(0, -1, 0.0)
        with self.assertRaises(ValueError):
            array.SetComponent(0, 2, 0.0)
        with self.assertRaises(ValueError):
            array.SetComponent(-1, 0, 0.0)
        with self.assertRaises(ValueError):
            array.SetComponent(3, 1, 0.0)

if __name__ == "__main__":
    Testing.main([(TestExpects, 'test')])
