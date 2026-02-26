"""Test repr, equality, and arithmetic operators for vtkTuple-derived types."""

from vtkmodules.vtkCommonDataModel import (
    vtkColor3d,
    vtkColor3f,
    vtkColor3ub,
    vtkColor4d,
    vtkColor4f,
    vtkColor4ub,
    vtkVector2d,
    vtkVector2f,
    vtkVector2i,
    vtkVector3d,
    vtkVector3f,
    vtkVector3i,
)
from vtkmodules.vtkCommonMath import vtkQuaterniond, vtkQuaternionf
from vtkmodules.test import Testing


class TestTupleOperators(Testing.vtkTest):
    """Tests for Python operators on vtkTuple-derived types."""

    # ---- repr ----

    def test_repr_vector3d(self):
        self.assertEqual(repr(vtkVector3d(1, 2, 3)), "vtkVector3d(1.0, 2.0, 3.0)")

    def test_repr_vector3i(self):
        self.assertEqual(repr(vtkVector3i(1, 2, 3)), "vtkVector3i(1, 2, 3)")

    def test_repr_vector2d(self):
        self.assertEqual(repr(vtkVector2d(1.5, 2.5)), "vtkVector2d(1.5, 2.5)")

    def test_repr_color3ub(self):
        self.assertEqual(repr(vtkColor3ub(255, 128, 0)), "vtkColor3ub(255, 128, 0)")

    def test_repr_color4ub(self):
        self.assertEqual(
            repr(vtkColor4ub(255, 128, 0, 200)), "vtkColor4ub(255, 128, 0, 200)"
        )

    def test_repr_color3d(self):
        self.assertEqual(repr(vtkColor3d(1.0, 0.5, 0.0)), "vtkColor3d(1.0, 0.5, 0.0)")

    def test_repr_quaterniond(self):
        self.assertEqual(
            repr(vtkQuaterniond(1, 0, 0, 0)), "vtkQuaterniond(1.0, 0.0, 0.0, 0.0)"
        )

    # ---- equality ----

    def test_vector3d_eq(self):
        self.assertTrue(vtkVector3d(1, 2, 3) == vtkVector3d(1, 2, 3))

    def test_vector3d_ne(self):
        self.assertTrue(vtkVector3d(1, 2, 3) != vtkVector3d(4, 5, 6))

    def test_vector3d_eq_false(self):
        self.assertFalse(vtkVector3d(1, 2, 3) == vtkVector3d(4, 5, 6))

    def test_vector3d_ne_false(self):
        self.assertFalse(vtkVector3d(1, 2, 3) != vtkVector3d(1, 2, 3))

    def test_cross_type_eq(self):
        """Different types should not be equal even with same values."""
        self.assertFalse(vtkVector3d(1, 2, 3) == vtkVector3f(1, 2, 3))

    def test_cross_type_ne(self):
        self.assertTrue(vtkVector3d(1, 2, 3) != vtkVector3f(1, 2, 3))

    def test_color_eq(self):
        self.assertTrue(vtkColor3ub(255, 0, 0) == vtkColor3ub(255, 0, 0))

    def test_color_ne(self):
        self.assertTrue(vtkColor3ub(255, 0, 0) != vtkColor3ub(0, 255, 0))

    def test_quaternion_eq(self):
        self.assertTrue(vtkQuaterniond(1, 0, 0, 0) == vtkQuaterniond(1, 0, 0, 0))

    def test_quaternion_ne(self):
        self.assertTrue(vtkQuaterniond(1, 0, 0, 0) != vtkQuaterniond(0, 1, 0, 0))

    def test_vector2d_eq(self):
        self.assertTrue(vtkVector2d(1, 2) == vtkVector2d(1, 2))

    def test_vector3i_eq(self):
        self.assertTrue(vtkVector3i(1, 2, 3) == vtkVector3i(1, 2, 3))

    # ---- vector arithmetic ----

    def test_add(self):
        result = vtkVector3d(1, 2, 3) + vtkVector3d(4, 5, 6)
        self.assertEqual(result, vtkVector3d(5, 7, 9))

    def test_subtract(self):
        result = vtkVector3d(4, 5, 6) - vtkVector3d(1, 1, 1)
        self.assertEqual(result, vtkVector3d(3, 4, 5))

    def test_multiply_elementwise(self):
        result = vtkVector3d(1, 2, 3) * vtkVector3d(2, 3, 4)
        self.assertEqual(result, vtkVector3d(2, 6, 12))

    def test_divide_elementwise(self):
        result = vtkVector3d(4, 6, 8) / vtkVector3d(2, 3, 4)
        self.assertEqual(result, vtkVector3d(2, 2, 2))

    def test_negate(self):
        result = -vtkVector3d(1, 2, 3)
        self.assertEqual(result, vtkVector3d(-1, -2, -3))

    def test_scalar_multiply(self):
        result = vtkVector3d(1, 2, 3) * 2.0
        self.assertEqual(result, vtkVector3d(2, 4, 6))

    def test_scalar_multiply_reverse(self):
        result = 2.0 * vtkVector3d(1, 2, 3)
        self.assertEqual(result, vtkVector3d(2, 4, 6))

    def test_scalar_divide(self):
        result = vtkVector3d(4, 6, 8) / 2.0
        self.assertEqual(result, vtkVector3d(2, 3, 4))

    def test_type_preservation(self):
        """Arithmetic results should preserve the concrete type."""
        result = vtkVector3d(1, 2, 3) + vtkVector3d(4, 5, 6)
        self.assertIsInstance(result, vtkVector3d)

    def test_int_vector_add(self):
        result = vtkVector3i(1, 2, 3) + vtkVector3i(4, 5, 6)
        self.assertEqual(result, vtkVector3i(5, 7, 9))

    def test_int_vector_negate(self):
        result = -vtkVector3i(1, 2, 3)
        self.assertEqual(result, vtkVector3i(-1, -2, -3))

    def test_vector2d_add(self):
        result = vtkVector2d(1, 2) + vtkVector2d(3, 4)
        self.assertEqual(result, vtkVector2d(4, 6))

    def test_vector2i_multiply_scalar(self):
        result = vtkVector2i(3, 4) * 2
        self.assertEqual(result, vtkVector2i(6, 8))

    # ---- color arithmetic ----

    def test_color_add(self):
        result = vtkColor3d(0.25, 0.5, 0.75) + vtkColor3d(0.25, 0.5, 0.25)
        self.assertEqual(result, vtkColor3d(0.5, 1.0, 1.0))

    def test_color_scalar_add(self):
        result = vtkColor3d(0, 0, 1) + 1
        self.assertIsInstance(result, vtkColor3d)
        self.assertEqual(result, vtkColor3d(1, 1, 2))

    def test_color_negate(self):
        result = -vtkColor3d(1.0, 0.5, 0.0)
        self.assertEqual(result, vtkColor3d(-1.0, -0.5, 0.0))

    def test_color_scalar_multiply(self):
        result = vtkColor3d(0.5, 0.25, 0.0) * 2.0
        self.assertEqual(result, vtkColor3d(1.0, 0.5, 0.0))

    # ---- incompatible types ----

    def test_add_incompatible(self):
        with self.assertRaises(TypeError):
            vtkVector3d(1, 2, 3) + "hello"

    def test_add_different_vector_types(self):
        """Adding vectors of different element types should fail."""
        with self.assertRaises(TypeError):
            vtkVector3d(1, 2, 3) + vtkVector3i(4, 5, 6)

    # ---- quaternion arithmetic ----

    def test_quaternion_add(self):
        result = vtkQuaterniond(1, 0, 0, 0) + vtkQuaterniond(0, 1, 0, 0)
        self.assertEqual(result, vtkQuaterniond(1, 1, 0, 0))

    def test_quaternion_subtract(self):
        result = vtkQuaterniond(1, 1, 0, 0) - vtkQuaterniond(0, 1, 0, 0)
        self.assertEqual(result, vtkQuaterniond(1, 0, 0, 0))

    def test_quaternion_multiply(self):
        """Quaternion multiplication (Hamilton product)."""
        result = vtkQuaterniond(1, 0, 0, 0) * vtkQuaterniond(0, 1, 0, 0)
        self.assertEqual(result, vtkQuaterniond(0, 1, 0, 0))

    # ---- slice indexing ----

    def test_slice_basic(self):
        v = vtkVector3d(1, 2, 3)
        self.assertEqual(v[0:2], (1.0, 2.0))

    def test_slice_full(self):
        v = vtkVector3d(1, 2, 3)
        self.assertEqual(v[:], (1.0, 2.0, 3.0))

    def test_slice_step(self):
        c = vtkColor4ub(10, 20, 30, 40)
        self.assertEqual(c[::2], (10, 30))

    def test_slice_reverse(self):
        c = vtkColor4ub(10, 20, 30, 40)
        self.assertEqual(c[::-1], (40, 30, 20, 10))

    def test_slice_empty(self):
        v = vtkVector3d(1, 2, 3)
        self.assertEqual(v[2:0], ())

    def test_negative_index(self):
        v = vtkVector3d(1, 2, 3)
        self.assertEqual(v[-1], 3.0)
        self.assertEqual(v[-2], 2.0)

    def test_slice_color3d(self):
        c = vtkColor3d(0.1, 0.2, 0.3)
        self.assertEqual(c[1:], (0.2, 0.3))

    def test_slice_out_of_range(self):
        """Slices clamp to bounds, but integer index raises."""
        v = vtkVector3d(1, 2, 3)
        self.assertEqual(v[0:10], (1.0, 2.0, 3.0))
        with self.assertRaises(IndexError):
            v[3]

    # ---- color conversions ----

    def test_color3ub_to_double(self):
        c = vtkColor3ub(255, 128, 0)
        d = c.ToDouble()
        self.assertIsInstance(d, vtkColor3d)
        self.assertAlmostEqual(d[0], 1.0, places=5)
        self.assertAlmostEqual(d[1], 128 / 255.0, places=5)
        self.assertAlmostEqual(d[2], 0.0, places=5)

    def test_color3ub_to_float(self):
        c = vtkColor3ub(255, 128, 0)
        f = c.ToFloat()
        self.assertIsInstance(f, vtkColor3f)
        self.assertAlmostEqual(f[0], 1.0, places=5)
        self.assertAlmostEqual(f[1], 128 / 255.0, places=5)
        self.assertAlmostEqual(f[2], 0.0, places=5)

    def test_color3d_to_unsigned_char(self):
        d = vtkColor3d(1.0, 128 / 255.0, 0.0)
        c = d.ToUnsignedChar()
        self.assertIsInstance(c, vtkColor3ub)
        self.assertEqual(c[0], 255)
        self.assertEqual(c[1], 128)
        self.assertEqual(c[2], 0)

    def test_color3d_to_float(self):
        d = vtkColor3d(0.5, 0.25, 1.0)
        f = d.ToFloat()
        self.assertIsInstance(f, vtkColor3f)
        self.assertAlmostEqual(f[0], 0.5, places=5)
        self.assertAlmostEqual(f[1], 0.25, places=5)
        self.assertAlmostEqual(f[2], 1.0, places=5)

    def test_color3f_to_unsigned_char(self):
        f = vtkColor3f(1.0, 0.5, 0.0)
        c = f.ToUnsignedChar()
        self.assertIsInstance(c, vtkColor3ub)
        self.assertEqual(c[0], 255)
        self.assertEqual(c[1], 128)
        self.assertEqual(c[2], 0)

    def test_color3f_to_double(self):
        f = vtkColor3f(0.5, 0.25, 1.0)
        d = f.ToDouble()
        self.assertIsInstance(d, vtkColor3d)
        self.assertAlmostEqual(d[0], 0.5, places=5)
        self.assertAlmostEqual(d[1], 0.25, places=5)
        self.assertAlmostEqual(d[2], 1.0, places=5)

    def test_color3ub_round_trip_via_double(self):
        """ub -> double -> ub should be lossless for integer values."""
        c = vtkColor3ub(100, 200, 50)
        self.assertEqual(c.ToDouble().ToUnsignedChar(), c)

    def test_color3ub_round_trip_via_float(self):
        """ub -> float -> ub should be lossless for integer values."""
        c = vtkColor3ub(100, 200, 50)
        self.assertEqual(c.ToFloat().ToUnsignedChar(), c)

    def test_color3d_clamping_high(self):
        """Values > 1.0 should be clamped to 255."""
        d = vtkColor3d(1.5, 2.0, 1.0)
        c = d.ToUnsignedChar()
        self.assertEqual(c[0], 255)
        self.assertEqual(c[1], 255)
        self.assertEqual(c[2], 255)

    def test_color3d_clamping_low(self):
        """Negative values should be clamped to 0."""
        d = vtkColor3d(-0.5, -1.0, 0.0)
        c = d.ToUnsignedChar()
        self.assertEqual(c[0], 0)
        self.assertEqual(c[1], 0)
        self.assertEqual(c[2], 0)

    def test_color4ub_to_double(self):
        c = vtkColor4ub(255, 128, 0, 200)
        d = c.ToDouble()
        self.assertIsInstance(d, vtkColor4d)
        self.assertAlmostEqual(d[0], 1.0, places=5)
        self.assertAlmostEqual(d[1], 128 / 255.0, places=5)
        self.assertAlmostEqual(d[2], 0.0, places=5)
        self.assertAlmostEqual(d[3], 200 / 255.0, places=5)

    def test_color4ub_to_float(self):
        c = vtkColor4ub(255, 128, 0, 200)
        f = c.ToFloat()
        self.assertIsInstance(f, vtkColor4f)
        self.assertAlmostEqual(f[0], 1.0, places=5)
        self.assertAlmostEqual(f[1], 128 / 255.0, places=5)
        self.assertAlmostEqual(f[2], 0.0, places=5)
        self.assertAlmostEqual(f[3], 200 / 255.0, places=5)

    def test_color4d_to_unsigned_char(self):
        d = vtkColor4d(1.0, 0.5, 0.0, 200 / 255.0)
        c = d.ToUnsignedChar()
        self.assertIsInstance(c, vtkColor4ub)
        self.assertEqual(c[0], 255)
        self.assertEqual(c[1], 128)
        self.assertEqual(c[2], 0)
        self.assertEqual(c[3], 200)

    def test_color4d_to_float(self):
        d = vtkColor4d(0.5, 0.25, 1.0, 0.75)
        f = d.ToFloat()
        self.assertIsInstance(f, vtkColor4f)
        self.assertAlmostEqual(f[0], 0.5, places=5)
        self.assertAlmostEqual(f[1], 0.25, places=5)
        self.assertAlmostEqual(f[2], 1.0, places=5)
        self.assertAlmostEqual(f[3], 0.75, places=5)

    def test_color4f_to_unsigned_char(self):
        f = vtkColor4f(1.0, 0.5, 0.0, 200 / 255.0)
        c = f.ToUnsignedChar()
        self.assertIsInstance(c, vtkColor4ub)
        self.assertEqual(c[0], 255)
        self.assertEqual(c[1], 128)
        self.assertEqual(c[2], 0)
        self.assertEqual(c[3], 200)

    def test_color4f_to_double(self):
        f = vtkColor4f(0.5, 0.25, 1.0, 0.75)
        d = f.ToDouble()
        self.assertIsInstance(d, vtkColor4d)
        self.assertAlmostEqual(d[0], 0.5, places=5)
        self.assertAlmostEqual(d[1], 0.25, places=5)
        self.assertAlmostEqual(d[2], 1.0, places=5)
        self.assertAlmostEqual(d[3], 0.75, places=5)

    def test_color4ub_round_trip_via_double(self):
        """4-component ub -> double -> ub round-trip."""
        c = vtkColor4ub(100, 200, 50, 180)
        self.assertEqual(c.ToDouble().ToUnsignedChar(), c)

    def test_color4f_clamping(self):
        """4-component clamping for out-of-range values."""
        f = vtkColor4f(1.5, -0.5, 0.5, 2.0)
        c = f.ToUnsignedChar()
        self.assertEqual(c[0], 255)
        self.assertEqual(c[1], 0)
        self.assertEqual(c[2], 128)
        self.assertEqual(c[3], 255)

    # ---- hash ----

    def test_unhashable(self):
        """Mutable sequence types should not be hashable."""
        with self.assertRaises(TypeError):
            hash(vtkVector3d(1, 2, 3))

    # ---- ordered comparisons ----

    def test_vector_less_than_raises(self):
        """Ordered comparison on tuple-only types raises TypeError."""
        with self.assertRaises(TypeError):
            vtkVector3d(1, 2, 3) < vtkVector3d(4, 5, 6)

    def test_color_greater_than_raises(self):
        """Ordered comparison on color types raises TypeError."""
        with self.assertRaises(TypeError):
            vtkColor3d(0.1, 0.2, 0.3) >= vtkColor3d(0.4, 0.5, 0.6)

    # ---- reflected arithmetic with non-special LHS ----

    def test_tuple_plus_vector_raises(self):
        """A plain tuple on the left of + should raise (no element-wise broadcast)."""
        with self.assertRaises(TypeError):
            (1, 2, 3) + vtkVector3d(4, 5, 6)

    def test_list_minus_vector_raises(self):
        """A plain list on the left of - should raise."""
        with self.assertRaises(TypeError):
            [1, 2, 3] - vtkVector3d(4, 5, 6)

    # ---- slice assignment ----

    def test_slice_assignment_raises(self):
        """Slice assignment is not supported (mp_ass_subscript is null)."""
        v = vtkVector3d(1, 2, 3)
        with self.assertRaises(TypeError):
            v[0:2] = (10, 20)

    def test_index_assignment_works(self):
        """Single-index assignment does work via the existing setter."""
        v = vtkVector3d(1, 2, 3)
        v[0] = 10.0
        self.assertEqual(v[0], 10.0)

    # ---- IEEE NaN ----

    def test_nan_not_equal_self(self):
        """Equality goes through Python rich-compare, so NaN != NaN element-wise."""
        nan = float("nan")
        v = vtkVector3d(nan, 2, 3)
        self.assertFalse(v == v)
        self.assertTrue(v != v)


if __name__ == "__main__":
    Testing.main([(TestTupleOperators, "test")])
