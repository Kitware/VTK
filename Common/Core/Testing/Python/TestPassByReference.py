"""Test the vtk.reference() type and test pass-by-reference.

Created on Sept 19, 2010 by David Gobbi
"""

import sys
import vtk
from vtk.test import Testing

class TestPassByReference(Testing.vtkTest):
    def testFloatReference(self):
        m = vtk.reference(3.0)
        n = vtk.reference(4.0)
        m *= 2
        self.assertEqual(m, 6.0)
        self.assertEqual(str(m), str(m.get()))
        o = n + m
        self.assertEqual(o, 10.0)

    def testIntReference(self):
        m = vtk.reference(3)
        n = vtk.reference(4)
        m |= n
        self.assertEqual(m, 7.0)
        self.assertEqual(str(m), str(m.get()))

    def testStringReference(self):
        m = vtk.reference("%s %s!")
        m %= ("hello", "world")
        self.assertEqual(m, "hello world!")

    def testTupleReference(self):
        m = vtk.reference((0,))
        self.assertEqual(m, (0,))
        self.assertEqual(len(m), 1)
        self.assertEqual(m[0], 0)

    def testPassByReferenceerence(self):
        t = vtk.reference(0.0)
        p0 = (0.5, 0.0, 0.0)
        n = (1.0, 0.0, 0.0)
        p1 = (0.0, 0.0, 0.0)
        p2 = (1.0, 1.0, 1.0)
        x = [0.0, 0.0, 0.0]
        vtk.vtkPlane.IntersectWithLine(p1, p2, n, p0, t, x)
        self.assertEqual(round(t,6), 0.5)
        self.assertEqual(round(x[0],6), 0.5)
        self.assertEqual(round(x[1],6), 0.5)
        self.assertEqual(round(x[2],6), 0.5)
        vtk.vtkPlane().IntersectWithLine(p1, p2, n, p0, t, x)
        self.assertEqual(round(t,6), 0.5)
        self.assertEqual(round(x[0],6), 0.5)
        self.assertEqual(round(x[1],6), 0.5)
        self.assertEqual(round(x[2],6), 0.5)
        t.set(0)
        p = vtk.vtkPlane()
        p.SetOrigin(0.5, 0.0, 0.0)
        p.SetNormal(1.0, 0.0, 0.0)
        p.IntersectWithLine(p1, p2, t, x)
        self.assertEqual(round(t,6), 0.5)
        self.assertEqual(round(x[0],6), 0.5)
        self.assertEqual(round(x[1],6), 0.5)
        self.assertEqual(round(x[2],6), 0.5)
        vtk.vtkPlane.IntersectWithLine(p, p1, p2, t, x)
        self.assertEqual(round(t,6), 0.5)
        self.assertEqual(round(x[0],6), 0.5)
        self.assertEqual(round(x[1],6), 0.5)
        self.assertEqual(round(x[2],6), 0.5)

    def testPassTupleByReference(self):
        n = vtk.reference(0)
        t = vtk.reference((0,))
        ca = vtk.vtkCellArray()
        ca.InsertNextCell(3, (1, 3, 0))
        ca.GetCell(0, n, t)
        self.assertEqual(n, 3)
        self.assertEqual(tuple(t), (1, 3, 0))

if __name__ == "__main__":
    Testing.main([(TestPassByReference, 'test')])
