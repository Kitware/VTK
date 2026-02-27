"""Tests for the Pythonic vtkVariantArray API."""

from vtkmodules.vtkCommonCore import vtkVariantArray
from vtkmodules.test import Testing


class TestVariantArrayPythonAPI(Testing.vtkTest):
    def test_empty_len(self):
        arr = vtkVariantArray()
        self.assertEqual(len(arr), 0)

    def test_append_and_len(self):
        arr = vtkVariantArray()
        arr.append(42)
        arr.append("hello")
        self.assertEqual(len(arr), 2)

    def test_getitem_positive(self):
        arr = vtkVariantArray()
        arr.append(1)
        arr.append("two")
        arr.append(3.0)
        self.assertEqual(arr[0], 1)
        self.assertEqual(arr[1], "two")
        self.assertAlmostEqual(arr[2], 3.0)

    def test_getitem_negative(self):
        arr = vtkVariantArray()
        arr.append(1)
        arr.append("two")
        arr.append(3.0)
        self.assertAlmostEqual(arr[-1], 3.0)
        self.assertEqual(arr[-2], "two")
        self.assertEqual(arr[-3], 1)

    def test_getitem_index_error(self):
        arr = vtkVariantArray()
        arr.append(42)
        with self.assertRaises(IndexError):
            arr[1]
        with self.assertRaises(IndexError):
            arr[-2]

    def test_setitem_positive(self):
        arr = vtkVariantArray()
        arr.append(1)
        arr.append(2)
        arr[0] = "x"
        arr[1] = 99
        self.assertEqual(arr[0], "x")
        self.assertEqual(arr[1], 99)

    def test_setitem_negative(self):
        arr = vtkVariantArray()
        arr.append(1)
        arr.append(2)
        arr[-1] = "z"
        self.assertEqual(arr[1], "z")

    def test_setitem_index_error(self):
        arr = vtkVariantArray()
        arr.append(42)
        with self.assertRaises(IndexError):
            arr[1] = "y"
        with self.assertRaises(IndexError):
            arr[-2] = "y"

    def test_contains(self):
        arr = vtkVariantArray()
        arr.append(42)
        arr.append("hello")
        self.assertTrue(42 in arr)
        self.assertTrue("hello" in arr)
        self.assertFalse("missing" in arr)
        self.assertFalse(99 in arr)

    def test_iter(self):
        arr = vtkVariantArray()
        arr.append(1)
        arr.append("two")
        arr.append(3.0)
        result = list(arr)
        self.assertEqual(result[0], 1)
        self.assertEqual(result[1], "two")
        self.assertAlmostEqual(result[2], 3.0)

    def test_extend(self):
        arr = vtkVariantArray()
        arr.extend([1, "two", 3.0])
        self.assertEqual(len(arr), 3)
        self.assertEqual(arr[0], 1)
        self.assertEqual(arr[1], "two")

    def test_clear(self):
        arr = vtkVariantArray()
        arr.extend([1, "two", 3.0])
        arr.clear()
        self.assertEqual(len(arr), 0)

    def test_repr_small(self):
        arr = vtkVariantArray()
        arr.extend([1, "hello", 3.14])
        r = repr(arr)
        self.assertIn("vtkVariantArray", r)
        self.assertIn("1", r)
        self.assertIn("'hello'", r)
        self.assertIn("3.14", r)

    def test_repr_large(self):
        arr = vtkVariantArray()
        for i in range(20):
            arr.append(i)
        r = repr(arr)
        self.assertIn("vtkVariantArray", r)
        self.assertIn("20 total", r)

    def test_mixed_types(self):
        """Verify that the array handles mixed types correctly."""
        arr = vtkVariantArray()
        arr.append(42)
        arr.append(3.14)
        arr.append("hello")
        self.assertIsInstance(arr[0], int)
        self.assertIsInstance(arr[1], float)
        self.assertIsInstance(arr[2], str)

    def test_traditional_api(self):
        """Verify traditional VTK API still works."""
        arr = vtkVariantArray()
        arr.InsertNextValue(42)
        arr.InsertNextValue("hello")
        self.assertEqual(arr.GetNumberOfValues(), 2)

    def test_isinstance(self):
        """Verify the override is a proper vtkVariantArray instance."""
        arr = vtkVariantArray()
        self.assertIsInstance(arr, vtkVariantArray)

    def test_constructor_from_list(self):
        arr = vtkVariantArray([1, "hello", 3.14])
        self.assertEqual(len(arr), 3)
        self.assertEqual(arr[0], 1)
        self.assertEqual(arr[1], "hello")
        self.assertAlmostEqual(arr[2], 3.14)

    def test_constructor_from_tuple(self):
        arr = vtkVariantArray((1, "hello"))
        self.assertEqual(len(arr), 2)
        self.assertEqual(arr[0], 1)
        self.assertEqual(arr[1], "hello")

    def test_constructor_from_iterator(self):
        arr = vtkVariantArray(iter([42]))
        self.assertEqual(len(arr), 1)
        self.assertEqual(arr[0], 42)

    def test_constructor_empty_iterable(self):
        arr = vtkVariantArray([])
        self.assertEqual(len(arr), 0)


if __name__ == "__main__":
    Testing.main([(TestVariantArrayPythonAPI, "test")])
