"""Tests for the Pythonic implicit functions API."""

import sys
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

# ---- Constructor kwargs ----


def test_sphere_kwargs():
    s = vtkSphere(center=(1, 2, 3), radius=5)
    assert tuple(s.GetCenter()) == (1.0, 2.0, 3.0)
    assert s.GetRadius() == 5.0


def test_sphere_defaults():
    s = vtkSphere()
    assert tuple(s.GetCenter()) == (0.0, 0.0, 0.0)
    assert s.GetRadius() == 0.5


def test_plane_kwargs():
    p = vtkPlane(origin=(1, 0, 0), normal=(0, 1, 0), offset=2.5)
    assert tuple(p.GetOrigin()) == (1.0, 0.0, 0.0)
    assert tuple(p.GetNormal()) == (0.0, 1.0, 0.0)
    assert p.GetOffset() == 2.5


def test_box_kwargs():
    b = vtkBox(bounds=(-1, 1, -2, 2, -3, 3))
    bounds = b.GetBounds()
    assert tuple(bounds) == (-1.0, 1.0, -2.0, 2.0, -3.0, 3.0)


def test_cylinder_kwargs():
    c = vtkCylinder(center=(1, 2, 3), axis=(0, 0, 1), radius=2.5)
    assert tuple(c.GetCenter()) == (1.0, 2.0, 3.0)
    assert tuple(c.GetAxis()) == (0.0, 0.0, 1.0)
    assert c.GetRadius() == 2.5


def test_cone_kwargs():
    c = vtkCone(origin=(1, 0, 0), axis=(0, 1, 0), angle=30)
    assert tuple(c.GetOrigin()) == (1.0, 0.0, 0.0)
    assert tuple(c.GetAxis()) == (0.0, 1.0, 0.0)
    assert c.GetAngle() == 30.0


def test_quadric_kwargs():
    coeffs = (1, 0, 0, 0, 1, 0, 0, 0, 1, -1)
    q = vtkQuadric(coefficients=coeffs)
    result = q.GetCoefficients()
    for i in range(10):
        assert result[i] == coeffs[i]


def test_superquadric_kwargs():
    sq = vtkSuperquadric(
        center=(1, 2, 3),
        phi_roundness=0.5,
        theta_roundness=0.3,
        toroidal=True,
    )
    assert tuple(sq.GetCenter()) == (1.0, 2.0, 3.0)
    assert sq.GetPhiRoundness() == 0.5
    assert sq.GetThetaRoundness() == 0.3
    assert sq.GetToroidal()


def test_perlin_noise_kwargs():
    pn = vtkPerlinNoise(frequency=(2, 3, 4), phase=(0.1, 0.2, 0.3), amplitude=5.0)
    assert tuple(pn.GetFrequency()) == (2.0, 3.0, 4.0)
    assert tuple(pn.GetPhase()) == (0.1, 0.2, 0.3)
    assert pn.GetAmplitude() == 5.0


def test_implicit_boolean_kwargs():
    b = vtkImplicitBoolean(operation_type="intersection")
    assert b.GetOperationType() == 1


def test_implicit_boolean_bad_op():
    try:
        vtkImplicitBoolean(operation_type="invalid")
        assert False, "Expected ValueError"
    except ValueError:
        pass


def test_implicit_sum_kwargs():
    s = vtkImplicitSum(normalize_by_weight=True)
    assert s.GetNormalizeByWeight()


def test_annulus_kwargs():
    a = vtkAnnulus(center=(1, 0, 0), inner_radius=2.0, outer_radius=5.0)
    assert tuple(a.GetCenter()) == (1.0, 0.0, 0.0)
    assert a.GetInnerRadius() == 2.0
    assert a.GetOuterRadius() == 5.0


def test_frustum_kwargs():
    f = vtkFrustum(
        near_plane_distance=0.5, horizontal_angle=45.0, vertical_angle=30.0
    )
    assert f.GetNearPlaneDistance() == 0.5
    assert f.GetHorizontalAngle() == 45.0
    assert f.GetVerticalAngle() == 30.0


def test_implicit_halo_kwargs():
    h = vtkImplicitHalo(center=(1, 2, 3), radius=4.0, fade_out=0.5)
    assert tuple(h.GetCenter()) == (1.0, 2.0, 3.0)
    assert h.GetRadius() == 4.0
    assert h.GetFadeOut() == 0.5


def test_coordinate_frame_kwargs():
    cf = vtkCoordinateFrame(
        origin=(1, 0, 0), x_axis=(1, 0, 0), y_axis=(0, 1, 0), z_axis=(0, 0, 1)
    )
    assert tuple(cf.GetOrigin()) == (1.0, 0.0, 0.0)
    assert tuple(cf.GetXAxis()) == (1.0, 0.0, 0.0)


# ---- __repr__ ----


def test_sphere_repr():
    s = vtkSphere(center=(1, 2, 3), radius=5)
    r = repr(s)
    assert "vtkSphere" in r
    assert "radius=5" in r


def test_plane_repr():
    p = vtkPlane(origin=(0, 0, 0), normal=(0, 0, 1))
    r = repr(p)
    assert "vtkPlane" in r
    assert "normal=" in r


def test_box_repr():
    b = vtkBox(bounds=(-1, 1, -2, 2, -3, 3))
    r = repr(b)
    assert "vtkBox" in r
    assert "bounds=" in r


def test_cylinder_repr():
    c = vtkCylinder(radius=3)
    r = repr(c)
    assert "vtkCylinder" in r
    assert "radius=3" in r


def test_cone_repr():
    c = vtkCone(angle=45)
    r = repr(c)
    assert "vtkCone" in r
    assert "angle=45" in r


def test_quadric_repr():
    q = vtkQuadric()
    r = repr(q)
    assert "vtkQuadric" in r
    assert "coefficients=" in r


def test_superquadric_repr():
    sq = vtkSuperquadric()
    r = repr(sq)
    assert "vtkSuperquadric" in r


def test_perlin_noise_repr():
    pn = vtkPerlinNoise()
    r = repr(pn)
    assert "vtkPerlinNoise" in r


def test_implicit_boolean_repr():
    b = vtkImplicitBoolean(operation_type="union")
    r = repr(b)
    assert "vtkImplicitBoolean" in r
    assert "Union" in r


def test_implicit_sum_repr():
    s = vtkImplicitSum()
    r = repr(s)
    assert "vtkImplicitSum" in r


def test_annulus_repr():
    a = vtkAnnulus(inner_radius=1, outer_radius=3)
    r = repr(a)
    assert "vtkAnnulus" in r


def test_frustum_repr():
    f = vtkFrustum()
    r = repr(f)
    assert "vtkFrustum" in r


def test_implicit_halo_repr():
    h = vtkImplicitHalo()
    r = repr(h)
    assert "vtkImplicitHalo" in r


def test_coordinate_frame_repr():
    cf = vtkCoordinateFrame()
    r = repr(cf)
    assert "vtkCoordinateFrame" in r


def test_tier3_repr():
    # Tier 3 classes get generic repr
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
        r = repr(obj)
        assert obj.GetClassName() in r


# ---- __call__ (callable protocol) ----


def test_call_three_scalars():
    s = vtkSphere(center=(0, 0, 0), radius=1)
    val = s(0, 0, 0)
    assert isinstance(val, float)
    assert val < 0  # inside sphere


def test_call_single_point():
    s = vtkSphere(center=(0, 0, 0), radius=1)
    val = s([0, 0, 0])
    assert isinstance(val, float)
    assert val < 0


def test_call_batch_array():
    s = vtkSphere(center=(0, 0, 0), radius=1)
    pts = np.array([[0, 0, 0], [2, 0, 0], [0, 0, 0.5]], dtype=np.float64)
    result = s(pts)
    assert isinstance(result, np.ndarray)
    assert result.shape == (3,)
    assert result[0] < 0  # inside
    assert result[1] > 0  # outside


def test_call_bad_args():
    s = vtkSphere()
    try:
        s(1, 2)
        assert False, "Expected TypeError"
    except TypeError:
        pass


# ---- CSG operators ----


def test_union():
    s = vtkSphere(center=(0, 0, 0), radius=1)
    p = vtkPlane(origin=(0, 0, 0), normal=(1, 0, 0))
    result = s | p
    assert isinstance(result, vtkImplicitBoolean)
    assert result.GetOperationType() == 0  # VTK_UNION
    assert result.GetFunction().GetNumberOfItems() == 2


def test_intersection():
    s = vtkSphere(center=(0, 0, 0), radius=1)
    p = vtkPlane(origin=(0, 0, 0), normal=(1, 0, 0))
    result = s & p
    assert isinstance(result, vtkImplicitBoolean)
    assert result.GetOperationType() == 1  # VTK_INTERSECTION
    assert result.GetFunction().GetNumberOfItems() == 2


def test_difference():
    s = vtkSphere(center=(0, 0, 0), radius=1)
    p = vtkPlane(origin=(0, 0, 0), normal=(1, 0, 0))
    result = s - p
    assert isinstance(result, vtkImplicitBoolean)
    assert result.GetOperationType() == 2  # VTK_DIFFERENCE


def test_negate():
    s = vtkSphere(center=(0, 0, 0), radius=1)
    result = ~s
    assert isinstance(result, vtkImplicitSum)
    # Negated sphere should flip sign
    inside_val = s(0, 0, 0)
    negated_val = result(0, 0, 0)
    assert abs(negated_val + inside_val) < 1e-10


def test_chaining():
    a = vtkSphere(center=(0, 0, 0), radius=1)
    b = vtkPlane(origin=(0, 0, 0), normal=(1, 0, 0))
    c = vtkCylinder(radius=0.5)
    result = (a & b) - c
    assert isinstance(result, vtkImplicitBoolean)
    assert result.GetOperationType() == 2  # difference
    # The result should be callable
    val = result(0.0, 0.0, 0.0)
    assert isinstance(val, float)


def test_double_negate():
    s = vtkSphere(center=(0, 0, 0), radius=1)
    result = ~~s
    assert isinstance(result, vtkImplicitSum)
    orig_val = s(0, 0, 0)
    double_neg_val = result(0, 0, 0)
    assert abs(double_neg_val - orig_val) < 1e-10


def test_not_implemented_for_non_implicit():
    s = vtkSphere()
    assert s.__or__(42) is NotImplemented
    assert s.__and__("str") is NotImplemented
    assert s.__sub__(3.14) is NotImplemented


def test_csg_result_is_callable():
    s = vtkSphere(center=(0, 0, 0), radius=1)
    p = vtkPlane(origin=(0, 0, 0), normal=(1, 0, 0))
    union = s | p
    val = union(0.5, 0, 0)
    assert isinstance(val, float)


def test_csg_result_has_repr():
    s = vtkSphere(center=(0, 0, 0), radius=1)
    p = vtkPlane(origin=(0, 0, 0), normal=(1, 0, 0))
    union = s | p
    r = repr(union)
    assert "vtkImplicitBoolean" in r
    assert "Union" in r
    assert "functions=2" in r


# ---- Run all tests ----


def _run_tests():
    failures = 0
    test_funcs = [
        (name, obj)
        for name, obj in sorted(globals().items())
        if name.startswith("test_") and callable(obj)
    ]
    for name, func in test_funcs:
        try:
            func()
            print("  PASS: %s" % name)
        except Exception as e:
            print("  FAIL: %s: %s" % (name, e))
            import traceback

            traceback.print_exc()
            failures += 1
    return failures


if __name__ == "__main__":
    print("Testing Pythonic implicit functions API...")
    failures = _run_tests()
    if failures:
        print("\n%d test(s) FAILED" % failures)
        sys.exit(1)
    else:
        print("\nAll tests passed.")
        sys.exit(0)
