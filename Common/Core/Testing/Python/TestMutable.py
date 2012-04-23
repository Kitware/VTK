"""Test the vtk.mutable() type and test pass-by-reference.

Created on Sept 19, 2010 by David Gobbi
"""

import sys
import vtk
from vtk.test import Testing

class TestMutable(Testing.vtkTest):
    def testFloatMutable(self):
        m = vtk.mutable(3.0)
        n = vtk.mutable(4.0)
        m *= 2
        self.assertEqual(m, 6.0)
        self.assertEqual(str(m), str(m.get()))
        o = n + m
        self.assertEqual(o, 10.0)

    def testIntMutable(self):
        m = vtk.mutable(3)
        n = vtk.mutable(4)
        m |= n
        self.assertEqual(m, 7.0)
        self.assertEqual(str(m), str(m.get()))

    def testStringMutable(self):
        m = vtk.mutable("%s %s!")
        m %= ("hello", "world")
        self.assertEqual(m, "hello world!")

    def testPassByReference(self):
        t = vtk.mutable(0.0)
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
        t.set(0)
        p = vtk.vtkPlane()
        p.SetOrigin(0.5, 0.0, 0.0)
        p.SetNormal(1.0, 0.0, 0.0)
        p.IntersectWithLine(p1, p2, t, x)
        self.assertEqual(round(t,6), 0.5)
        self.assertEqual(round(x[0],6), 0.5)
        self.assertEqual(round(x[1],6), 0.5)
        self.assertEqual(round(x[2],6), 0.5)

if __name__ == "__main__":
    Testing.main([(TestMutable, 'test')])
