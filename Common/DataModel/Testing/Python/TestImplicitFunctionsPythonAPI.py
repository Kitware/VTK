"""Tests for the Pythonic implicit functions API."""

import unittest

import numpy as np
from vtkmodules.vtkCommonDataModel import (
    vtkAnnulus,
    vtkBox,
    vtkCone,
    vtkCoordinateFrame,
    vtkCylinder,
    vtkFrustum,
    vtkImplicitBoolean,
    vtkImplicitDataSet,
    vtkImplicitFunction,
    vtkImplicitHalo,
    vtkImplicitSelectionLoop,
    vtkImplicitSum,
    vtkImplicitVolume,
    vtkImplicitWindowFunction,
    vtkPerlinNoise,
    vtkPlane,
    vtkPlanes,
    vtkPolyPlane,
    vtkQuadric,
    vtkSphere,
    vtkSpheres,
    vtkSuperquadric,
)

try:
    from vtkmodules.vtkFiltersCore import vtkImplicitPolyDataDistance  # noqa: F401

    _HAS_FILTERS_CORE = True
except ImportError:
    _HAS_FILTERS_CORE = False


class TestConstructorKwargs(unittest.TestCase):
    def test_sphere_kwargs(self):
        s = vtkSphere(center=(1, 2, 3), radius=5)
        self.assertEqual(tuple(s.GetCenter()), (1.0, 2.0, 3.0))
        self.assertEqual(s.GetRadius(), 5.0)

    def test_sphere_defaults(self):
        s = vtkSphere()
        self.assertEqual(tuple(s.GetCenter()), (0.0, 0.0, 0.0))
        self.assertEqual(s.GetRadius(), 0.5)

    def test_plane_kwargs(self):
        p = vtkPlane(origin=(1, 0, 0), normal=(0, 1, 0), offset=2.5)
        self.assertEqual(tuple(p.GetOrigin()), (1.0, 0.0, 0.0))
        self.assertEqual(tuple(p.GetNormal()), (0.0, 1.0, 0.0))
        self.assertEqual(p.GetOffset(), 2.5)

    def test_box_kwargs(self):
        b = vtkBox(bounds=(-1, 1, -2, 2, -3, 3))
        self.assertEqual(tuple(b.GetBounds()), (-1.0, 1.0, -2.0, 2.0, -3.0, 3.0))

    def test_cylinder_kwargs(self):
        c = vtkCylinder(center=(1, 2, 3), axis=(0, 0, 1), radius=2.5)
        self.assertEqual(tuple(c.GetCenter()), (1.0, 2.0, 3.0))
        self.assertEqual(tuple(c.GetAxis()), (0.0, 0.0, 1.0))
        self.assertEqual(c.GetRadius(), 2.5)

    def test_cone_kwargs(self):
        c = vtkCone(origin=(1, 0, 0), axis=(0, 1, 0), angle=30)
        self.assertEqual(tuple(c.GetOrigin()), (1.0, 0.0, 0.0))
        self.assertEqual(tuple(c.GetAxis()), (0.0, 1.0, 0.0))
        self.assertEqual(c.GetAngle(), 30.0)

    def test_quadric_kwargs(self):
        coeffs = (1, 0, 0, 0, 1, 0, 0, 0, 1, -1)
        q = vtkQuadric(coefficients=coeffs)
        result = q.GetCoefficients()
        for i in range(10):
            self.assertEqual(result[i], coeffs[i])

    def test_superquadric_kwargs(self):
        sq = vtkSuperquadric(
            center=(1, 2, 3),
            phi_roundness=0.5,
            theta_roundness=0.3,
            toroidal=True,
        )
        self.assertEqual(tuple(sq.GetCenter()), (1.0, 2.0, 3.0))
        self.assertEqual(sq.GetPhiRoundness(), 0.5)
        self.assertEqual(sq.GetThetaRoundness(), 0.3)
        self.assertTrue(sq.GetToroidal())

    def test_perlin_noise_kwargs(self):
        pn = vtkPerlinNoise(frequency=(2, 3, 4), phase=(0.1, 0.2, 0.3), amplitude=5.0)
        self.assertEqual(tuple(pn.GetFrequency()), (2.0, 3.0, 4.0))
        self.assertEqual(tuple(pn.GetPhase()), (0.1, 0.2, 0.3))
        self.assertEqual(pn.GetAmplitude(), 5.0)

    def test_implicit_boolean_kwargs(self):
        # The enum constant maps to SetOperationType via the wrapper.
        b = vtkImplicitBoolean(operation_type=vtkImplicitBoolean.VTK_INTERSECTION)
        self.assertEqual(b.GetOperationType(), vtkImplicitBoolean.VTK_INTERSECTION)

    def test_implicit_sum_kwargs(self):
        s = vtkImplicitSum(normalize_by_weight=True)
        self.assertTrue(s.GetNormalizeByWeight())

    def test_annulus_kwargs(self):
        a = vtkAnnulus(center=(1, 0, 0), inner_radius=2.0, outer_radius=5.0)
        self.assertEqual(tuple(a.GetCenter()), (1.0, 0.0, 0.0))
        self.assertEqual(a.GetInnerRadius(), 2.0)
        self.assertEqual(a.GetOuterRadius(), 5.0)

    def test_frustum_kwargs(self):
        f = vtkFrustum(
            near_plane_distance=0.5, horizontal_angle=45.0, vertical_angle=30.0
        )
        self.assertEqual(f.GetNearPlaneDistance(), 0.5)
        self.assertEqual(f.GetHorizontalAngle(), 45.0)
        self.assertEqual(f.GetVerticalAngle(), 30.0)

    def test_implicit_halo_kwargs(self):
        h = vtkImplicitHalo(center=(1, 2, 3), radius=4.0, fade_out=0.5)
        self.assertEqual(tuple(h.GetCenter()), (1.0, 2.0, 3.0))
        self.assertEqual(h.GetRadius(), 4.0)
        self.assertEqual(h.GetFadeOut(), 0.5)

    def test_coordinate_frame_kwargs(self):
        cf = vtkCoordinateFrame(
            origin=(1, 0, 0), x_axis=(1, 0, 0), y_axis=(0, 1, 0), z_axis=(0, 0, 1)
        )
        self.assertEqual(tuple(cf.GetOrigin()), (1.0, 0.0, 0.0))
        self.assertEqual(tuple(cf.GetXAxis()), (1.0, 0.0, 0.0))


class TestRepr(unittest.TestCase):
    def test_sphere_repr(self):
        r = repr(vtkSphere(center=(1, 2, 3), radius=5))
        self.assertIn("vtkSphere", r)
        self.assertIn("radius=5", r)

    def test_plane_repr(self):
        r = repr(vtkPlane(origin=(0, 0, 0), normal=(0, 0, 1)))
        self.assertIn("vtkPlane", r)
        self.assertIn("normal=", r)

    def test_box_repr(self):
        r = repr(vtkBox(bounds=(-1, 1, -2, 2, -3, 3)))
        self.assertIn("vtkBox", r)
        self.assertIn("bounds=", r)

    def test_cylinder_repr(self):
        r = repr(vtkCylinder(radius=3))
        self.assertIn("vtkCylinder", r)
        self.assertIn("radius=3", r)

    def test_cone_repr(self):
        r = repr(vtkCone(angle=45))
        self.assertIn("vtkCone", r)
        self.assertIn("angle=45", r)

    def test_quadric_repr(self):
        r = repr(vtkQuadric())
        self.assertIn("vtkQuadric", r)
        self.assertIn("coefficients=", r)

    def test_superquadric_repr(self):
        self.assertIn("vtkSuperquadric", repr(vtkSuperquadric()))

    def test_perlin_noise_repr(self):
        self.assertIn("vtkPerlinNoise", repr(vtkPerlinNoise()))

    def test_implicit_boolean_repr(self):
        r = repr(vtkImplicitBoolean(operation_type=vtkImplicitBoolean.VTK_UNION))
        self.assertIn("vtkImplicitBoolean", r)
        self.assertIn("Union", r)

    def test_implicit_sum_repr(self):
        self.assertIn("vtkImplicitSum", repr(vtkImplicitSum()))

    def test_annulus_repr(self):
        self.assertIn("vtkAnnulus", repr(vtkAnnulus(inner_radius=1, outer_radius=3)))

    def test_frustum_repr(self):
        self.assertIn("vtkFrustum", repr(vtkFrustum()))

    def test_implicit_halo_repr(self):
        self.assertIn("vtkImplicitHalo", repr(vtkImplicitHalo()))

    def test_coordinate_frame_repr(self):
        self.assertIn("vtkCoordinateFrame", repr(vtkCoordinateFrame()))

    def test_tier3_repr(self):
        # Tier 3 classes get a generic repr.
        for cls in [
            vtkImplicitWindowFunction,
            vtkImplicitSelectionLoop,
            vtkImplicitDataSet,
            vtkImplicitVolume,
            vtkPlanes,
            vtkSpheres,
            vtkPolyPlane,
        ]:
            obj = cls()
            self.assertIn(obj.GetClassName(), repr(obj))


class TestCallable(unittest.TestCase):
    def test_call_three_scalars(self):
        s = vtkSphere(center=(0, 0, 0), radius=1)
        val = s(0, 0, 0)
        self.assertIsInstance(val, float)
        self.assertLess(val, 0)  # inside sphere

    def test_call_single_point(self):
        s = vtkSphere(center=(0, 0, 0), radius=1)
        val = s([0, 0, 0])
        self.assertIsInstance(val, float)
        self.assertLess(val, 0)

    def test_call_batch_array(self):
        s = vtkSphere(center=(0, 0, 0), radius=1)
        pts = np.array([[0, 0, 0], [2, 0, 0], [0, 0, 0.5]], dtype=np.float64)
        result = s(pts)
        self.assertIsInstance(result, np.ndarray)
        self.assertEqual(result.shape, (3,))
        self.assertLess(result[0], 0)  # inside
        self.assertGreater(result[1], 0)  # outside

    def test_call_batch_list_of_lists(self):
        # A plain Python Nx3 sequence should work like a numpy array.
        s = vtkSphere(center=(0, 0, 0), radius=1)
        result = s([[0, 0, 0], [2, 0, 0], [0, 0, 0.5]])
        self.assertIsInstance(result, np.ndarray)
        self.assertEqual(result.shape, (3,))
        self.assertLess(result[0], 0)  # inside
        self.assertGreater(result[1], 0)  # outside

    def test_call_bad_args(self):
        s = vtkSphere()
        with self.assertRaises(TypeError):
            s(1, 2)


class TestCSGOperators(unittest.TestCase):
    def test_union(self):
        s = vtkSphere(center=(0, 0, 0), radius=1)
        p = vtkPlane(origin=(0, 0, 0), normal=(1, 0, 0))
        result = s | p
        self.assertIsInstance(result, vtkImplicitBoolean)
        self.assertEqual(result.GetOperationType(), vtkImplicitBoolean.VTK_UNION)
        self.assertEqual(result.GetFunction().GetNumberOfItems(), 2)

    def test_intersection(self):
        s = vtkSphere(center=(0, 0, 0), radius=1)
        p = vtkPlane(origin=(0, 0, 0), normal=(1, 0, 0))
        result = s & p
        self.assertIsInstance(result, vtkImplicitBoolean)
        self.assertEqual(result.GetOperationType(), vtkImplicitBoolean.VTK_INTERSECTION)
        self.assertEqual(result.GetFunction().GetNumberOfItems(), 2)

    def test_difference(self):
        s = vtkSphere(center=(0, 0, 0), radius=1)
        p = vtkPlane(origin=(0, 0, 0), normal=(1, 0, 0))
        result = s - p
        self.assertIsInstance(result, vtkImplicitBoolean)
        self.assertEqual(result.GetOperationType(), vtkImplicitBoolean.VTK_DIFFERENCE)

    def test_negate(self):
        s = vtkSphere(center=(0, 0, 0), radius=1)
        result = ~s
        self.assertIsInstance(result, vtkImplicitSum)
        # Negated sphere should flip sign.
        inside_val = s(0, 0, 0)
        negated_val = result(0, 0, 0)
        self.assertAlmostEqual(negated_val + inside_val, 0.0, places=10)

    def test_chaining(self):
        a = vtkSphere(center=(0, 0, 0), radius=1)
        b = vtkPlane(origin=(0, 0, 0), normal=(1, 0, 0))
        c = vtkCylinder(radius=0.5)
        result = (a & b) - c
        self.assertIsInstance(result, vtkImplicitBoolean)
        self.assertEqual(result.GetOperationType(), vtkImplicitBoolean.VTK_DIFFERENCE)
        self.assertIsInstance(result(0.0, 0.0, 0.0), float)

    def test_double_negate(self):
        s = vtkSphere(center=(0, 0, 0), radius=1)
        result = ~~s
        self.assertIsInstance(result, vtkImplicitSum)
        orig_val = s(0, 0, 0)
        double_neg_val = result(0, 0, 0)
        self.assertAlmostEqual(double_neg_val, orig_val, places=10)

    def test_reflected_operators(self):
        # Reflected operators build the boolean with operands in swapped order.
        s = vtkSphere(center=(0, 0, 0), radius=1)
        p = vtkPlane(origin=(0, 0, 0), normal=(1, 0, 0))
        union = s.__ror__(p)
        self.assertIsInstance(union, vtkImplicitBoolean)
        self.assertEqual(union.GetOperationType(), vtkImplicitBoolean.VTK_UNION)
        inter = s.__rand__(p)
        self.assertEqual(inter.GetOperationType(), vtkImplicitBoolean.VTK_INTERSECTION)
        diff = s.__rsub__(p)
        self.assertEqual(diff.GetOperationType(), vtkImplicitBoolean.VTK_DIFFERENCE)
        self.assertEqual(diff.GetFunction().GetNumberOfItems(), 2)

    @unittest.skipUnless(_HAS_FILTERS_CORE, "vtkFiltersCore not available")
    def test_reflected_operator_dispatch(self):
        # When the left operand is a vtkImplicitFunction that the override
        # layer does not cover (here vtkImplicitPolyDataDistance), it has no
        # __or__, so Python routes the expression to the right operand's
        # __ror__. This is the case that makes the reflected operators worth
        # having: it lets CSG expressions mix in implicit functions defined
        # outside this module.
        from vtkmodules.vtkFiltersCore import vtkImplicitPolyDataDistance

        other = vtkImplicitPolyDataDistance()
        self.assertNotIn("__or__", vars(type(other)))  # no override mixin
        s = vtkSphere(center=(0, 0, 0), radius=1)
        result = other | s
        self.assertIsInstance(result, vtkImplicitBoolean)
        self.assertEqual(result.GetOperationType(), vtkImplicitBoolean.VTK_UNION)
        self.assertEqual(result.GetFunction().GetNumberOfItems(), 2)

    def test_reflected_not_implemented_for_non_implicit(self):
        s = vtkSphere()
        self.assertIs(s.__ror__(42), NotImplemented)
        self.assertIs(s.__rand__("str"), NotImplemented)
        self.assertIs(s.__rsub__(3.14), NotImplemented)

    def test_not_implemented_for_non_implicit(self):
        s = vtkSphere()
        self.assertIs(s.__or__(42), NotImplemented)
        self.assertIs(s.__and__("str"), NotImplemented)
        self.assertIs(s.__sub__(3.14), NotImplemented)

    def test_csg_result_is_callable(self):
        s = vtkSphere(center=(0, 0, 0), radius=1)
        p = vtkPlane(origin=(0, 0, 0), normal=(1, 0, 0))
        union = s | p
        self.assertIsInstance(union(0.5, 0, 0), float)

    def test_csg_result_has_repr(self):
        s = vtkSphere(center=(0, 0, 0), radius=1)
        p = vtkPlane(origin=(0, 0, 0), normal=(1, 0, 0))
        r = repr(s | p)
        self.assertIn("vtkImplicitBoolean", r)
        self.assertIn("Union", r)


if __name__ == "__main__":
    unittest.main()
