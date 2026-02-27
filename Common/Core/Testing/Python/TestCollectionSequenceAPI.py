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

    def test_append(self):
        c = vtkCollection()
        a = vtkObject()
        b = vtkObject()
        c.append(a)
        c.append(b)
        self.assertEqual(len(c), 2)
        self.assertIs(c[0], a)
        self.assertIs(c[1], b)

    def test_insert(self):
        c = vtkCollection()
        a = vtkObject()
        b = vtkObject()
        d = vtkObject()
        c.append(a)
        c.append(b)
        c.insert(1, d)
        self.assertEqual(len(c), 3)
        self.assertIs(c[0], a)
        self.assertIs(c[1], d)
        self.assertIs(c[2], b)

    def test_remove(self):
        c = vtkCollection()
        a = vtkObject()
        b = vtkObject()
        c.append(a)
        c.append(b)
        c.remove(a)
        self.assertEqual(len(c), 1)
        self.assertIs(c[0], b)

    def test_clear(self):
        c = vtkCollection()
        for _ in range(5):
            c.append(vtkObject())
        self.assertEqual(len(c), 5)
        c.clear()
        self.assertEqual(len(c), 0)

    def test_append_type_error(self):
        c = vtkCollection()
        with self.assertRaises(TypeError):
            c.append(42)
        with self.assertRaises(TypeError):
            c.append("hello")

    def test_methods_inherited_by_subclass(self):
        """Subclasses inherit append/insert/remove/clear."""
        dac = vtkDataArrayCollection()
        arr = vtkIntArray()
        dac.append(arr)
        self.assertEqual(len(dac), 1)
        self.assertIs(dac[0], arr)
        dac.clear()
        self.assertEqual(len(dac), 0)


if __name__ == "__main__":
    Testing.main([(TestCollectionSequenceAPI, 'test')])
