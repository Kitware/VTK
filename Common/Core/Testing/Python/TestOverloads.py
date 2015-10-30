"""Test overloaded method resolution in VTK-Python

The wrappers should call overloaded C++ methods using similar
overload resolution rules as C++.  Python itself does not have
method overloading.

Created on Feb 15, 2015 by David Gobbi

"""

import sys
import vtk
from vtk.test import Testing

class TestOverloads(Testing.vtkTest):

    def testMethods(self):
        """Test overloaded methods"""
        # single-argument method vtkTransform::SetMatrix()
        t = vtk.vtkTransform()
        m = vtk.vtkMatrix4x4()
        m.SetElement(0, 0, 2)
        t.SetMatrix(m)
        self.assertEqual(t.GetMatrix().GetElement(0, 0), 2)
        t.SetMatrix([0,1,0,0, 1,0,0,0, 0,0,-1,0, 0,0,0,1])
        self.assertEqual(t.GetMatrix().GetElement(0, 0), 0)
        # mixed number of arguments
        fd = vtk.vtkFieldData()
        fa = vtk.vtkFloatArray()
        fa.SetName("Real")
        ia = vtk.vtkIntArray()
        ia.SetName("Integer")
        fd.AddArray(fa)
        fd.AddArray(ia)
        a = fd.GetArray("Real")
        self.assertEqual(id(a), id(fa))
        i = vtk.mutable(0)
        a = fd.GetArray("Integer", i)
        self.assertEqual(id(a), id(ia))
        self.assertEqual(i, 1)

    def testConstructors(self):
        """Test overloaded constructors"""
        # resolve by number of arguments
        v = vtk.vtkVector3d(3, 4, 5)
        self.assertEqual((v[0], v[1], v[2]), (3, 4, 5))
        v = vtk.vtkVector3d(6)
        self.assertEqual((v[0], v[1], v[2]), (6, 6, 6))
        # resolve by argument type
        v = vtk.vtkVariant(3.0)
        self.assertEqual(v.GetType(), vtk.VTK_DOUBLE)
        v = vtk.vtkVariant(1)
        self.assertEqual(v.GetType(), vtk.VTK_INT)
        v = vtk.vtkVariant("hello")
        self.assertEqual(v.GetType(), vtk.VTK_STRING)
        v = vtk.vtkVariant(vtk.vtkObject())
        self.assertEqual(v.GetType(), vtk.VTK_OBJECT)

    def testArgumentConversion(self):
        """Test argument conversion via implicit constructors"""
        # automatic conversion to vtkVariant
        a = vtk.vtkVariantArray()
        a.InsertNextValue(2.5)
        a.InsertNextValue(vtk.vtkObject())
        self.assertEqual(a.GetValue(0), vtk.vtkVariant(2.5))
        self.assertEqual(a.GetValue(1).GetType(), vtk.VTK_OBJECT)
        # same, but this one is via "const vtkVariant&" argument
        a = vtk.vtkDenseArray[float]()
        a.Resize(1)
        a.SetVariantValue(0, 2.5)
        self.assertEqual(a.GetVariantValue(0).ToDouble(), 2.5)


if __name__ == "__main__":
    Testing.main([(TestOverloads, 'test')])
