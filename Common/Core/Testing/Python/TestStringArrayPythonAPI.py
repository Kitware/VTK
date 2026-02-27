"""Tests for the Pythonic vtkStringArray API."""

from vtkmodules.vtkCommonCore import vtkStringArray
from vtkmodules.test import Testing


class TestStringArrayPythonAPI(Testing.vtkTest):
    def test_empty_len(self):
        arr = vtkStringArray()
        self.assertEqual(len(arr), 0)

    def test_append_and_len(self):
        arr = vtkStringArray()
        arr.append("hello")
        arr.append("world")
        self.assertEqual(len(arr), 2)

    def test_getitem_positive(self):
        arr = vtkStringArray()
        arr.append("a")
        arr.append("b")
        arr.append("c")
        self.assertEqual(arr[0], "a")
        self.assertEqual(arr[1], "b")
        self.assertEqual(arr[2], "c")

    def test_getitem_negative(self):
        arr = vtkStringArray()
        arr.append("a")
        arr.append("b")
        arr.append("c")
        self.assertEqual(arr[-1], "c")
        self.assertEqual(arr[-2], "b")
        self.assertEqual(arr[-3], "a")

    def test_getitem_index_error(self):
        arr = vtkStringArray()
        arr.append("x")
        with self.assertRaises(IndexError):
            arr[1]
        with self.assertRaises(IndexError):
            arr[-2]

    def test_setitem_positive(self):
        arr = vtkStringArray()
        arr.append("a")
        arr.append("b")
        arr[0] = "x"
        arr[1] = "y"
        self.assertEqual(arr[0], "x")
        self.assertEqual(arr[1], "y")

    def test_setitem_negative(self):
        arr = vtkStringArray()
        arr.append("a")
        arr.append("b")
        arr[-1] = "z"
        self.assertEqual(arr[1], "z")

    def test_setitem_index_error(self):
        arr = vtkStringArray()
        arr.append("x")
        with self.assertRaises(IndexError):
            arr[1] = "y"
        with self.assertRaises(IndexError):
            arr[-2] = "y"

    def test_contains(self):
        arr = vtkStringArray()
        arr.append("hello")
        arr.append("world")
        self.assertTrue("hello" in arr)
        self.assertTrue("world" in arr)
        self.assertFalse("missing" in arr)

    def test_iter(self):
        arr = vtkStringArray()
        arr.append("a")
        arr.append("b")
        arr.append("c")
        self.assertEqual(list(arr), ["a", "b", "c"])

    def test_extend(self):
        arr = vtkStringArray()
        arr.extend(["a", "b", "c"])
        self.assertEqual(len(arr), 3)
        self.assertEqual(list(arr), ["a", "b", "c"])

    def test_clear(self):
        arr = vtkStringArray()
        arr.extend(["a", "b", "c"])
        arr.clear()
        self.assertEqual(len(arr), 0)

    def test_repr_small(self):
        arr = vtkStringArray()
        arr.extend(["a", "b", "c"])
        r = repr(arr)
        self.assertIn("vtkStringArray", r)
        self.assertIn("'a'", r)
        self.assertIn("'b'", r)
        self.assertIn("'c'", r)

    def test_repr_large(self):
        arr = vtkStringArray()
        for i in range(20):
            arr.append("item%d" % i)
        r = repr(arr)
        self.assertIn("vtkStringArray", r)
        self.assertIn("20 total", r)

    def test_unicode(self):
        arr = vtkStringArray()
        arr.append("hello")
        arr.append("\u00e9\u00e0\u00fc")
        arr.append("\u4e16\u754c")
        self.assertEqual(arr[0], "hello")
        self.assertEqual(arr[1], "\u00e9\u00e0\u00fc")
        self.assertEqual(arr[2], "\u4e16\u754c")

    def test_traditional_api(self):
        """Verify traditional VTK API still works."""
        arr = vtkStringArray()
        arr.InsertNextValue("a")
        arr.InsertNextValue("b")
        self.assertEqual(arr.GetValue(0), "a")
        self.assertEqual(arr.GetValue(1), "b")
        arr.SetValue(0, "x")
        self.assertEqual(arr.GetValue(0), "x")
        self.assertEqual(arr.GetNumberOfValues(), 2)

    def test_isinstance(self):
        """Verify the override is a proper vtkStringArray instance."""
        arr = vtkStringArray()
        self.assertIsInstance(arr, vtkStringArray)

    def test_constructor_from_list(self):
        arr = vtkStringArray(["a", "b", "c"])
        self.assertEqual(len(arr), 3)
        self.assertEqual(list(arr), ["a", "b", "c"])

    def test_constructor_from_tuple(self):
        arr = vtkStringArray(("a", "b"))
        self.assertEqual(len(arr), 2)
        self.assertEqual(list(arr), ["a", "b"])

    def test_constructor_from_iterator(self):
        arr = vtkStringArray(iter(["x"]))
        self.assertEqual(len(arr), 1)
        self.assertEqual(arr[0], "x")

    def test_constructor_empty_iterable(self):
        arr = vtkStringArray([])
        self.assertEqual(len(arr), 0)


if __name__ == "__main__":
    Testing.main([(TestStringArrayPythonAPI, "test")])
