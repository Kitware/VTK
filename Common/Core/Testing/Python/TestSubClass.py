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
from vtkmodules.vtkCommonCore import (
    vtkCollection,
    vtkDataArray,
    vtkIntArray,
    vtkObject,
    vtkObjectBase,
    vtkPoints,
)
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.test import Testing

class vtkCustomObject(vtkObject):
    def __init__(self, extra=None):
        """Initialize all attributes."""
        if extra is None:
            extra = vtkObject()
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
        if o == None or isinstance(o, vtkObjectBase):
            self._ExtraObject = o
            self.Modified()
        else:
            raise TypeError("requires None or a vtkobject")

    def GetMTime(self):
        """Override a method (only works when called from Python)"""
        t = vtkObject.GetMTime(self)
        if self._ExtraObject:
            t = max(t, self._ExtraObject.GetMTime())
        return t

class vtkPointsCustom(vtkPoints):
    def __init__(self):
        self.some_attribute = "custom"

class TestSubclass(Testing.vtkTest):
    def testSubclassInstantiate(self):
        """Instantiate a python vtkObject subclass"""
        o = vtkCustomObject()
        self.assertEqual(o.GetClassName(), "vtkCustomObject")

    def testConstructorArgs(self):
        """Test the use of constructor arguments."""
        extra = vtkObject()
        o = vtkCustomObject(extra)
        self.assertEqual(o.GetClassName(), "vtkCustomObject")
        self.assertEqual(id(o.GetExtraObject()), id(extra))

    def testCallUnboundMethods(self):
        """Test calling an unbound method in an overridden method"""
        o = vtkCustomObject()
        a = vtkIntArray()
        o.SetExtraObject(a)
        a.Modified()
        # GetMTime should return a's mtime
        self.assertEqual(o.GetMTime(), a.GetMTime())
        # calling the vtkObject mtime should give a lower MTime
        self.assertNotEqual(o.GetMTime(), vtkObject.GetMTime(o))
        # another couple quick unbound method check
        vtkDataArray.InsertNextTuple1(a, 2)
        self.assertEqual(a.GetTuple1(0), 2)

    def testPythonRTTI(self):
        """Test the python isinstance and issubclass methods """
        o = vtkCustomObject()
        d = vtkIntArray()
        self.assertEqual(True, isinstance(o, vtkObjectBase))
        self.assertEqual(True, isinstance(d, vtkObjectBase))
        self.assertEqual(True, isinstance(o, vtkCustomObject))
        self.assertEqual(False, isinstance(d, vtkCustomObject))
        self.assertEqual(False, isinstance(o, vtkDataArray))
        self.assertEqual(True, issubclass(vtkCustomObject, vtkObject))
        self.assertEqual(False, issubclass(vtkObject, vtkCustomObject))
        self.assertEqual(False, issubclass(vtkCustomObject, vtkDataArray))

    def testSubclassGhost(self):
        """Make sure ghosting of the class works"""
        o = vtkCustomObject()
        c = vtkCollection()
        c.AddItem(o)
        i = id(o)
        del o
        o = vtkObject()
        o = c.GetItemAsObject(0)
        # make sure the id has changed, but class the same
        self.assertEqual(o.__class__, vtkCustomObject)
        self.assertNotEqual(i, id(o))

    def testOverride(self):
        """Make sure that overwriting with a subclass works"""
        self.assertFalse(isinstance(vtkPoints(), vtkPointsCustom))
        # check that object has the correct class
        vtkPoints.override(vtkPointsCustom)
        self.assertTrue(isinstance(vtkPoints(), vtkPointsCustom))
        # check object created deep in c++
        source = vtkSphereSource()
        source.Update()
        points = source.GetOutput().GetPoints()
        self.assertTrue(isinstance(points, vtkPointsCustom))
        # check that __init__ is called
        self.assertEqual(points.some_attribute, "custom")
        # check that overrides can be removed
        vtkPoints.override(None)
        self.assertTrue(vtkPoints().__class__ == vtkPoints)

if __name__ == "__main__":
    Testing.main([(TestSubclass, 'test')])
