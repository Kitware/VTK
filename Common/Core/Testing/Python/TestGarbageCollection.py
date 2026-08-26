"""Test that reference cycles through a wrapper's __dict__ are collectable.

A PyVTKObject is allocated with PyObject_GC_New and its type declares
Py_TPFLAGS_HAVE_GC, so CPython's cyclic collector tracks it and asks it,
via tp_traverse, what it refers to.  The instance dictionary (vtk_dict)
has to be reported there: without it, a cycle that passes through an
attribute of a wrapped object looks externally referenced to the
collector and is never freed, with no diagnostic.

Every cycle tested here leaks against a wrapper whose PyVTKObject_Traverse
does not visit vtk_dict.

Created on Aug 17, 2026 by Eric Larson

"""

import gc
import weakref

from vtkmodules.vtkCommonCore import (
    vtkObject,
    vtkPoints,
    vtkVariantArray,
)
from vtkmodules.test import Testing


class vtkCustomObject(vtkObject):
    pass


class Holder:
    """Holds a VTK object, the way a cached accessor or adapter does."""

    def __init__(self, obj):
        self.obj = obj

    def callback(self, caller, event):
        pass


class TestGarbageCollection(Testing.vtkTest):
    def testSelfCycle(self):
        """An object stored as its own attribute is collectable."""
        o = vtkPoints()
        o.self_reference = o
        ref = weakref.ref(o)
        del o

        gc.collect()
        self.assertIsNone(ref())

    def testCycleThroughHelperObject(self):
        """A helper stored on the object and holding it back is collectable."""
        o = vtkPoints()
        o.helper = Holder(o)
        ref = weakref.ref(o)
        helper_ref = weakref.ref(o.helper)
        del o

        gc.collect()
        self.assertIsNone(ref())
        self.assertIsNone(helper_ref())

    def testCycleBetweenTwoVTKObjects(self):
        """A cycle whose only members are wrapped objects is collectable."""
        first = vtkPoints()
        second = vtkPoints()
        first.other = second
        second.other = first
        refs = [weakref.ref(first), weakref.ref(second)]
        del first, second

        gc.collect()
        self.assertEqual([ref() for ref in refs], [None, None])

    def testCycleWithObserver(self):
        """A cycle through both an observer and the dict is collectable."""
        o = vtkObject()
        helper = Holder(o)
        o.AddObserver('ModifiedEvent', helper.callback)
        o.helper = helper
        ref = weakref.ref(o)
        helper_ref = weakref.ref(helper)
        del o, helper

        gc.collect()
        self.assertIsNone(ref())
        self.assertIsNone(helper_ref())

    def testCycleInPythonSubclass(self):
        """The same cycle in a Python subclass of a VTK class."""
        o = vtkCustomObject()
        o.helper = Holder(o)
        ref = weakref.ref(o)
        del o

        gc.collect()
        self.assertIsNone(ref())

    def testNoCycleIsFreedByRefcount(self):
        """An object with attributes but no cycle needs no collection."""
        o = vtkPoints()
        o.attribute = 'hello'
        o.helper = Holder(vtkPoints())
        ref = weakref.ref(o)
        del o

        # no gc.collect() here: refcounting alone must free it
        self.assertIsNone(ref())

    def testAttributesStillSurviveRoundTrip(self):
        """Traversing the dict must not disturb the ghosting of attributes."""
        o = vtkObject()
        o.customattr = 'hello'
        a = vtkVariantArray()
        a.InsertNextValue(o)
        ref = weakref.ref(o)
        del o
        gc.collect()

        # the wrapper is gone, so the attribute can only come back through
        # the ghost map
        self.assertIsNone(ref())
        o = a.GetValue(0).ToVTKObject()
        self.assertEqual(o.customattr, 'hello')

    def testCycleIsCollectedWhileCppObjectSurvives(self):
        """A cycle is collected even when the C++ object stays alive."""
        o = vtkObject()
        o.helper = Holder(o)
        a = vtkVariantArray()
        a.InsertNextValue(o)
        ref = weakref.ref(o)
        del o

        gc.collect()
        self.assertIsNone(ref())

        # Breaking the cycle empties the dict before the wrapper is deleted,
        # so there is nothing left for the ghost map to preserve: attributes
        # that took part in a collected cycle do not survive the round trip.
        o = a.GetValue(0).ToVTKObject()
        self.assertFalse(hasattr(o, 'helper'))


if __name__ == "__main__":
    Testing.main([(TestGarbageCollection, 'test')])
