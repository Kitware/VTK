"""Test subclassing support in VTK-Python

VTK classes can be subclassed in Python.  There are
some caveats, such as:
 - protected items are inaccessible to the python class
 - virtual method calls from C++ are not propagated to python

To be tested:
 - make sure that subclassing works
 - make sure that unbound superclass methods can be called

Created on Sept 26, 2010 by David Gobbi

"""

import sys
import vtk
from vtk.test import Testing

class vtkCustomObject(vtk.vtkObject):
    def __init__(self, extra=None):
        """Initialize all attributes."""
        if extra is None:
            extra = vtk.vtkObject()
        self._ExtraObject = extra

    def GetClassName(self):
        """Get the class name."""
        return self.__class__.__name__

    def GetExtraObject(self):
        """Getter method."""
        return self._ExtraObject

    def SetExtraObject(self, o):
        """Setter method."""
        # make sure it is "None" or a vtkobject instance
        if o == None or isinstance(o, vtk.vtkObjectBase):
            self._ExtraObject = o
            self.Modified()
        else:
            raise TypeError("requires None or a vtkobject")

    def GetMTime(self):
        """Override a method (only works when called from Python)"""
        t = vtk.vtkObject.GetMTime(self)
        if self._ExtraObject:
            t = max(t, self._ExtraObject.GetMTime())
        return t


class TestSubclass(Testing.vtkTest):
    def testSubclassInstantiate(self):
        """Instantiate a python vtkObject subclass"""
        o = vtkCustomObject()
        self.assertEqual(o.GetClassName(), "vtkCustomObject")

    def testConstructorArgs(self):
        """Test the use of constructor arguments."""
        extra = vtk.vtkObject()
        o = vtkCustomObject(extra)
        self.assertEqual(o.GetClassName(), "vtkCustomObject")
        self.assertEqual(id(o.GetExtraObject()), id(extra))

    def testCallUnboundMethods(self):
        """Test calling an unbound method in an overridded method"""
        o = vtkCustomObject()
        a = vtk.vtkIntArray()
        o.SetExtraObject(a)
        a.Modified()
        # GetMTime should return a's mtime
        self.assertEqual(o.GetMTime(), a.GetMTime())
        # calling the vtkObject mtime should give a lower MTime
        self.assertNotEqual(o.GetMTime(), vtk.vtkObject.GetMTime(o))
        # another couple quick unbound method check
        vtk.vtkDataArray.InsertNextTuple1(a, 2)
        self.assertEqual(a.GetTuple1(0), 2)

    def testPythonRTTI(self):
        """Test the python isinstance and issubclass methods """
        o = vtkCustomObject()
        d = vtk.vtkIntArray()
        self.assertEqual(True, isinstance(o, vtk.vtkObjectBase))
        self.assertEqual(True, isinstance(d, vtk.vtkObjectBase))
        self.assertEqual(True, isinstance(o, vtkCustomObject))
        self.assertEqual(False, isinstance(d, vtkCustomObject))
        self.assertEqual(False, isinstance(o, vtk.vtkDataArray))
        self.assertEqual(True, issubclass(vtkCustomObject, vtk.vtkObject))
        self.assertEqual(False, issubclass(vtk.vtkObject, vtkCustomObject))
        self.assertEqual(False, issubclass(vtkCustomObject, vtk.vtkDataArray))

    def testSubclassGhost(self):
        """Make sure ghosting of the class works"""
        o = vtkCustomObject()
        c = vtk.vtkCollection()
        c.AddItem(o)
        i = id(o)
        del o
        o = vtk.vtkObject()
        o = c.GetItemAsObject(0)
        # make sure the id has changed, but class the same
        self.assertEqual(o.__class__, vtkCustomObject)
        self.assertNotEqual(i, id(o))

if __name__ == "__main__":
    Testing.main([(TestSubclass, 'test')])
