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

if __name__ == "__main__":
    Testing.main([(TestExpects, 'test')])
