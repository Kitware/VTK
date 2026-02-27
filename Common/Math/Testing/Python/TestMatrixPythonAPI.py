"""Tests for the Pythonic vtkMatrix3x3/vtkMatrix4x4 API."""

import numpy as np
from numpy.testing import assert_array_equal, assert_array_almost_equal

from vtkmodules.vtkCommonMath import vtkMatrix3x3, vtkMatrix4x4
from vtkmodules.test import Testing


class TestMatrix4x4(Testing.vtkTest):
    """Tests for vtkMatrix4x4 Pythonic API."""

    def test_isinstance(self):
        m = vtkMatrix4x4()
        self.assertIsInstance(m, vtkMatrix4x4)

    def test_identity_default(self):
        m = vtkMatrix4x4()
        for i in range(4):
            for j in range(4):
                expected = 1.0 if i == j else 0.0
                self.assertEqual(m[i, j], expected)

    def test_element_access(self):
        m = vtkMatrix4x4()
        m[1, 2] = 3.5
        self.assertEqual(m[1, 2], 3.5)
        self.assertEqual(m.GetElement(1, 2), 3.5)

    def test_element_is_scalar(self):
        m = vtkMatrix4x4()
        val = m[0, 0]
        self.assertIsInstance(val, float)

    def test_row_access(self):
        m = vtkMatrix4x4()
        row = m[0]
        self.assertIsInstance(row, np.ndarray)
        assert_array_equal(row, [1.0, 0.0, 0.0, 0.0])
        self.assertEqual(row.dtype, np.float64)

    def test_row_set(self):
        m = vtkMatrix4x4()
        m[2] = [5.0, 6.0, 7.0, 8.0]
        assert_array_equal(m[2], [5.0, 6.0, 7.0, 8.0])

    def test_negative_indexing(self):
        m = vtkMatrix4x4()
        m[0, 3] = 42.0
        self.assertEqual(m[0, -1], 42.0)
        self.assertEqual(m[-4, 3], 42.0)
        self.assertEqual(m[-4, -1], 42.0)
        # Negative row index for row access
        assert_array_equal(m[-4], [1.0, 0.0, 0.0, 42.0])

    def test_index_error(self):
        m = vtkMatrix4x4()
        with self.assertRaises(IndexError):
            _ = m[4, 0]
        with self.assertRaises(IndexError):
            _ = m[0, 4]
        with self.assertRaises(IndexError):
            _ = m[-5, 0]
        with self.assertRaises(IndexError):
            _ = m[0, -5]
        with self.assertRaises(IndexError):
            m[4, 0] = 1.0
        with self.assertRaises(IndexError):
            _ = m[4]
        with self.assertRaises(IndexError):
            m[4] = [1, 2, 3, 4]

    def test_len(self):
        m = vtkMatrix4x4()
        self.assertEqual(len(m), 4)

    def test_repr(self):
        m = vtkMatrix4x4()
        r = repr(m)
        self.assertIn("vtkMatrix4x4", r)
        self.assertIn("1.0", r)

    def test_eq(self):
        a = vtkMatrix4x4()
        b = vtkMatrix4x4()
        self.assertEqual(a, b)
        b[0, 1] = 99.0
        self.assertNotEqual(a, b)

    def test_ne(self):
        a = vtkMatrix4x4()
        b = vtkMatrix4x4()
        self.assertFalse(a != b)
        b[3, 3] = 2.0
        self.assertTrue(a != b)

    def test_matmul(self):
        a = vtkMatrix4x4()
        a[0, 3] = 1.0  # translation x=1
        b = vtkMatrix4x4()
        b[1, 3] = 2.0  # translation y=2
        c = a @ b
        self.assertIsInstance(c, vtkMatrix4x4)
        # Result should combine translations
        self.assertEqual(c[0, 3], 1.0)
        self.assertEqual(c[1, 3], 2.0)
        # Diagonal should still be identity
        for i in range(4):
            self.assertEqual(c[i, i], 1.0)

    def test_invert(self):
        m = vtkMatrix4x4()
        m[0, 3] = 5.0
        inv = ~m
        self.assertIsInstance(inv, vtkMatrix4x4)
        self.assertAlmostEqual(inv[0, 3], -5.0)
        # Original unchanged
        self.assertEqual(m[0, 3], 5.0)
        # inv @ m should be identity
        result = inv @ m
        for i in range(4):
            for j in range(4):
                expected = 1.0 if i == j else 0.0
                self.assertAlmostEqual(result[i, j], expected, places=10)

    def test_constructor_nested(self):
        data = [
            [1, 0, 0, 10],
            [0, 1, 0, 20],
            [0, 0, 1, 30],
            [0, 0, 0, 1],
        ]
        m = vtkMatrix4x4(data)
        self.assertIsInstance(m, vtkMatrix4x4)
        self.assertEqual(m[0, 3], 10.0)
        self.assertEqual(m[1, 3], 20.0)
        self.assertEqual(m[2, 3], 30.0)

    def test_constructor_flat(self):
        flat = (1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)
        m = vtkMatrix4x4(flat)
        self.assertIsInstance(m, vtkMatrix4x4)
        for i in range(4):
            for j in range(4):
                expected = 1.0 if i == j else 0.0
                self.assertEqual(m[i, j], expected)

    def test_constructor_bad_size(self):
        with self.assertRaises(ValueError):
            vtkMatrix4x4([1, 2, 3])
        with self.assertRaises(ValueError):
            vtkMatrix4x4([[1, 2], [3, 4], [5, 6], [7, 8]])

    def test_row_set_bad_size(self):
        m = vtkMatrix4x4()
        with self.assertRaises(ValueError):
            m[0] = [1, 2, 3]  # too few

    def test_cpp_api_still_works(self):
        m = vtkMatrix4x4()
        m.SetElement(1, 2, 7.0)
        self.assertEqual(m.GetElement(1, 2), 7.0)
        m.Identity()
        self.assertEqual(m.GetElement(1, 2), 0.0)

    # ---- slice indexing tests ------------------------------------------------
    def test_row_slice(self):
        """m[1:3] returns a 2D numpy array of rows 1 and 2."""
        m = vtkMatrix4x4()
        m[1, 1] = 5.0
        m[2, 2] = 7.0
        result = m[1:3]
        self.assertIsInstance(result, np.ndarray)
        self.assertEqual(result.shape, (2, 4))
        assert_array_equal(result[0], [0.0, 5.0, 0.0, 0.0])
        assert_array_equal(result[1], [0.0, 0.0, 7.0, 0.0])

    def test_row_slice_all(self):
        """m[:] returns full 4x4 matrix."""
        m = vtkMatrix4x4()
        result = m[:]
        self.assertEqual(result.shape, (4, 4))
        assert_array_equal(result, np.eye(4))

    def test_row_slice_step(self):
        """m[::2] returns rows 0 and 2."""
        m = vtkMatrix4x4()
        result = m[::2]
        self.assertEqual(result.shape, (2, 4))
        assert_array_equal(result[0], [1.0, 0.0, 0.0, 0.0])
        assert_array_equal(result[1], [0.0, 0.0, 1.0, 0.0])

    def test_col_slice(self):
        """m[0, 1:3] returns columns 1-2 of row 0."""
        m = vtkMatrix4x4()
        m[0, 1] = 2.0
        m[0, 2] = 3.0
        result = m[0, 1:3]
        self.assertIsInstance(result, np.ndarray)
        self.assertEqual(result.shape, (2,))
        assert_array_equal(result, [2.0, 3.0])

    def test_row_slice_single_col(self):
        """m[1:3, 0] returns a 1D column slice."""
        m = vtkMatrix4x4()
        result = m[1:3, 0]
        self.assertIsInstance(result, np.ndarray)
        self.assertEqual(result.shape, (2,))
        assert_array_equal(result, [0.0, 0.0])

    def test_submatrix(self):
        """m[1:3, 1:3] returns a 2x2 submatrix."""
        m = vtkMatrix4x4()
        result = m[1:3, 1:3]
        self.assertIsInstance(result, np.ndarray)
        self.assertEqual(result.shape, (2, 2))
        assert_array_equal(result, np.eye(2))

    def test_submatrix_full(self):
        """m[:, :] returns full matrix."""
        m = vtkMatrix4x4()
        result = m[:, :]
        self.assertEqual(result.shape, (4, 4))
        assert_array_equal(result, np.eye(4))

    def test_negative_slice(self):
        """Negative indices in slices."""
        m = vtkMatrix4x4()
        result = m[-2:]
        self.assertEqual(result.shape, (2, 4))
        assert_array_equal(result[0], [0.0, 0.0, 1.0, 0.0])
        assert_array_equal(result[1], [0.0, 0.0, 0.0, 1.0])

    # ---- slice setitem tests -------------------------------------------------
    def test_set_row_slice(self):
        """m[1:3] = 2D array sets rows 1 and 2."""
        m = vtkMatrix4x4()
        m[1:3] = [[1, 2, 3, 4], [5, 6, 7, 8]]
        assert_array_equal(m[1], [1.0, 2.0, 3.0, 4.0])
        assert_array_equal(m[2], [5.0, 6.0, 7.0, 8.0])
        # Rows 0 and 3 unchanged
        assert_array_equal(m[0], [1.0, 0.0, 0.0, 0.0])
        assert_array_equal(m[3], [0.0, 0.0, 0.0, 1.0])

    def test_set_col_slice(self):
        """m[0, 1:3] = [a, b] sets columns 1-2 of row 0."""
        m = vtkMatrix4x4()
        m[0, 1:3] = [9.0, 8.0]
        self.assertEqual(m[0, 1], 9.0)
        self.assertEqual(m[0, 2], 8.0)

    def test_set_row_slice_single_col(self):
        """m[1:3, 0] = [a, b] sets column 0 of rows 1-2."""
        m = vtkMatrix4x4()
        m[1:3, 0] = [3.0, 4.0]
        self.assertEqual(m[1, 0], 3.0)
        self.assertEqual(m[2, 0], 4.0)

    def test_set_submatrix(self):
        """m[1:3, 1:3] = 2D array sets a submatrix."""
        m = vtkMatrix4x4()
        m[1:3, 1:3] = [[10, 20], [30, 40]]
        self.assertEqual(m[1, 1], 10.0)
        self.assertEqual(m[1, 2], 20.0)
        self.assertEqual(m[2, 1], 30.0)
        self.assertEqual(m[2, 2], 40.0)
        # Rest unchanged
        self.assertEqual(m[0, 0], 1.0)
        self.assertEqual(m[3, 3], 1.0)

    def test_empty_slice(self):
        """m[2:2] returns empty array."""
        m = vtkMatrix4x4()
        result = m[2:2]
        self.assertIsInstance(result, np.ndarray)
        self.assertEqual(result.shape, (0, 4))


class TestMatrix3x3(Testing.vtkTest):
    """Tests for vtkMatrix3x3 Pythonic API."""

    def test_isinstance(self):
        m = vtkMatrix3x3()
        self.assertIsInstance(m, vtkMatrix3x3)

    def test_identity_default(self):
        m = vtkMatrix3x3()
        for i in range(3):
            for j in range(3):
                expected = 1.0 if i == j else 0.0
                self.assertEqual(m[i, j], expected)

    def test_element_access(self):
        m = vtkMatrix3x3()
        m[0, 2] = 4.5
        self.assertEqual(m[0, 2], 4.5)

    def test_row_access(self):
        m = vtkMatrix3x3()
        row = m[0]
        self.assertIsInstance(row, np.ndarray)
        assert_array_equal(row, [1.0, 0.0, 0.0])

    def test_row_set(self):
        m = vtkMatrix3x3()
        m[1] = [4.0, 5.0, 6.0]
        assert_array_equal(m[1], [4.0, 5.0, 6.0])

    def test_negative_indexing(self):
        m = vtkMatrix3x3()
        m[0, 2] = 99.0
        self.assertEqual(m[-3, -1], 99.0)

    def test_index_error(self):
        m = vtkMatrix3x3()
        with self.assertRaises(IndexError):
            _ = m[3, 0]
        with self.assertRaises(IndexError):
            _ = m[0, 3]

    def test_len(self):
        self.assertEqual(len(vtkMatrix3x3()), 3)

    def test_repr(self):
        r = repr(vtkMatrix3x3())
        self.assertIn("vtkMatrix3x3", r)

    def test_eq(self):
        a = vtkMatrix3x3()
        b = vtkMatrix3x3()
        self.assertEqual(a, b)
        b[0, 1] = 1.0
        self.assertNotEqual(a, b)

    def test_matmul(self):
        a = vtkMatrix3x3()
        a[0, 1] = 1.0
        b = vtkMatrix3x3()
        b[1, 0] = 1.0
        c = a @ b
        self.assertIsInstance(c, vtkMatrix3x3)
        # (I + E01) @ (I + E10) = I + E01 + E10 + E01*E10
        # E01*E10 = E00, so result[0,0] = 1+1 = 2
        self.assertEqual(c[0, 0], 2.0)

    def test_invert(self):
        m = vtkMatrix3x3()
        m[0, 1] = 2.0
        inv = ~m
        self.assertIsInstance(inv, vtkMatrix3x3)
        result = inv @ m
        for i in range(3):
            for j in range(3):
                expected = 1.0 if i == j else 0.0
                self.assertAlmostEqual(result[i, j], expected, places=10)

    def test_constructor_nested(self):
        m = vtkMatrix3x3([[1, 2, 3], [4, 5, 6], [7, 8, 9]])
        self.assertEqual(m[0, 0], 1.0)
        self.assertEqual(m[2, 2], 9.0)

    def test_constructor_flat(self):
        m = vtkMatrix3x3((1, 0, 0, 0, 1, 0, 0, 0, 1))
        for i in range(3):
            for j in range(3):
                expected = 1.0 if i == j else 0.0
                self.assertEqual(m[i, j], expected)

    def test_cpp_api_still_works(self):
        m = vtkMatrix3x3()
        m.SetElement(0, 1, 3.0)
        self.assertEqual(m.GetElement(0, 1), 3.0)
        m.Identity()
        self.assertEqual(m.GetElement(0, 1), 0.0)

    # ---- slice tests for 3x3 ------------------------------------------------
    def test_row_slice(self):
        m = vtkMatrix3x3()
        result = m[0:2]
        self.assertIsInstance(result, np.ndarray)
        self.assertEqual(result.shape, (2, 3))
        assert_array_equal(result[0], [1.0, 0.0, 0.0])
        assert_array_equal(result[1], [0.0, 1.0, 0.0])

    def test_col_slice(self):
        m = vtkMatrix3x3()
        m[0, 1] = 5.0
        result = m[0, 0:2]
        self.assertEqual(result.shape, (2,))
        assert_array_equal(result, [1.0, 5.0])

    def test_submatrix(self):
        m = vtkMatrix3x3()
        result = m[0:2, 0:2]
        self.assertEqual(result.shape, (2, 2))
        assert_array_equal(result, np.eye(2))

    def test_full_slice(self):
        m = vtkMatrix3x3()
        result = m[:]
        self.assertEqual(result.shape, (3, 3))
        assert_array_equal(result, np.eye(3))

    def test_set_submatrix(self):
        m = vtkMatrix3x3()
        m[0:2, 0:2] = [[10, 20], [30, 40]]
        self.assertEqual(m[0, 0], 10.0)
        self.assertEqual(m[0, 1], 20.0)
        self.assertEqual(m[1, 0], 30.0)
        self.assertEqual(m[1, 1], 40.0)
        # Corner unchanged
        self.assertEqual(m[2, 2], 1.0)


if __name__ == "__main__":
    Testing.main([(TestMatrix4x4, 'test'), (TestMatrix3x3, 'test')])
