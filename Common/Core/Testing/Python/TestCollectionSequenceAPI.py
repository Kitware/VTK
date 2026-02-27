"""Test sequence protocol (len, [], in) for vtkCollection and subclasses."""

from vtkmodules.vtkCommonCore import (
    vtkCollection,
    vtkDataArrayCollection,
    vtkFloatArray,
    vtkIntArray,
    vtkObject,
)
from vtkmodules.test import Testing


class TestCollectionSequenceAPI(Testing.vtkTest):
    def test_len(self):
        c = vtkCollection()
        self.assertEqual(len(c), 0)

        objs = [vtkObject() for _ in range(5)]
        for o in objs:
            c.AddItem(o)
        self.assertEqual(len(c), 5)

    def test_getitem(self):
        c = vtkCollection()
        objs = [vtkObject() for _ in range(5)]
        for o in objs:
            c.AddItem(o)

        # positive indexing
        for i in range(5):
            self.assertIs(c[i], objs[i])

        # negative indexing
        self.assertIs(c[-1], objs[4])
        self.assertIs(c[-5], objs[0])

        # out of range
        with self.assertRaises(IndexError):
            c[5]
        with self.assertRaises(IndexError):
            c[-6]

    def test_getitem_empty(self):
        c = vtkCollection()
        with self.assertRaises(IndexError):
            c[0]

    def test_contains(self):
        c = vtkCollection()
        a = vtkObject()
        b = vtkObject()
        c.AddItem(a)

        self.assertIn(a, c)
        self.assertNotIn(b, c)

    def test_contains_non_vtk(self):
        c = vtkCollection()
        # non-VTK objects should return False, not error
        self.assertNotIn(42, c)
        self.assertNotIn("hello", c)

    def test_iteration_still_works(self):
        """Ensure the existing tp_iter protocol still works."""
        c = vtkCollection()
        objs = [vtkObject() for _ in range(3)]
        for o in objs:
            c.AddItem(o)
        self.assertEqual(list(c), objs)

    def test_subclass_inherits_data_array_collection(self):
        """vtkDataArrayCollection (same module) inherits sequence protocol."""
        dac = vtkDataArrayCollection()
        arr1 = vtkIntArray()
        arr2 = vtkFloatArray()
        dac.AddItem(arr1)
        dac.AddItem(arr2)

        self.assertEqual(len(dac), 2)
        self.assertIs(dac[0], arr1)
        self.assertIs(dac[-1], arr2)
        self.assertIn(arr1, dac)
        self.assertNotIn(vtkIntArray(), dac)

        with self.assertRaises(IndexError):
            dac[2]

    def test_subclass_inherits_cross_module(self):
        """Cross-module subclass inherits sequence protocol."""
        try:
            from vtkmodules.vtkRenderingCore import vtkRendererCollection, vtkRenderer
        except ImportError:
            self.skipTest("vtkRenderingCore not available")

        rc = vtkRendererCollection()
        r1 = vtkRenderer()
        r2 = vtkRenderer()
        rc.AddItem(r1)
        rc.AddItem(r2)

        self.assertEqual(len(rc), 2)
        self.assertIs(rc[0], r1)
        self.assertIs(rc[-1], r2)
        self.assertIn(r1, rc)
        self.assertNotIn(vtkRenderer(), rc)


if __name__ == "__main__":
    Testing.main([(TestCollectionSequenceAPI, 'test')])
