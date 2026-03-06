# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
"""Test the functions module (composite-aware algorithms)."""

import sys

try:
    import numpy as np
except ImportError:
    import vtkmodules.test.Testing
    print("This test requires numpy!")
    vtkmodules.test.Testing.skip()

from vtkmodules.vtkCommonCore import vtkFloatArray, vtkDoubleArray
from vtkmodules.vtkCommonDataModel import (
    vtkImageData, vtkMultiBlockDataSet, vtkPolyData
)
from vtkmodules.vtkImagingCore import vtkRTAnalyticSource
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkFiltersCore import vtkElevationFilter
from vtkmodules.numpy_interface.utils import NoneArray, ArrayAssociation
from vtkmodules.numpy_interface.vtk_partitioned_array import VTKPartitionedArray
from vtkmodules.util import functions, numpy_support

errors = 0


def check(condition, msg):
    global errors
    if not condition:
        print("ERROR:", msg)
        errors += 1


def check_close(actual, expected, msg, rtol=1e-5, atol=1e-8):
    if isinstance(actual, np.ndarray) and isinstance(expected, np.ndarray):
        if actual.shape != expected.shape:
            check(False, f"{msg}: shape mismatch {actual.shape} vs {expected.shape}")
            return
        if not np.allclose(actual, expected, rtol=rtol, atol=atol):
            check(False, f"{msg}: arrays not close\n  actual:   {actual}\n  expected: {expected}")
    elif np.isscalar(actual) and np.isscalar(expected):
        import math
        check(math.isclose(float(actual), float(expected), rel_tol=rtol, abs_tol=atol),
              f"{msg}: {actual} != {expected}")
    else:
        if not np.allclose(np.asarray(actual), np.asarray(expected), rtol=rtol, atol=atol):
            check(False, f"{msg}: {actual} != {expected}")


# -------------------------------------------------------------------
# Helpers: create test datasets
# -------------------------------------------------------------------
def make_image_dataset():
    """Create a small vtkImageData with Elevation as point scalar."""
    src = vtkRTAnalyticSource()
    src.SetWholeExtent(0, 5, 0, 5, 0, 5)
    src.Update()

    elev = vtkElevationFilter()
    elev.SetInputConnection(src.GetOutputPort())
    elev.SetLowPoint(0, 0, 0)
    elev.SetHighPoint(5, 5, 5)
    elev.Update()

    return elev.GetOutput()


def make_multiblock():
    """Create a vtkMultiBlockDataSet with 2 image blocks."""
    mb = vtkMultiBlockDataSet()

    src1 = vtkRTAnalyticSource()
    src1.SetWholeExtent(0, 3, 0, 3, 0, 3)
    src1.Update()
    elev1 = vtkElevationFilter()
    elev1.SetInputConnection(src1.GetOutputPort())
    elev1.SetLowPoint(0, 0, 0)
    elev1.SetHighPoint(3, 3, 3)
    elev1.Update()

    src2 = vtkRTAnalyticSource()
    src2.SetWholeExtent(0, 4, 0, 4, 0, 4)
    src2.Update()
    elev2 = vtkElevationFilter()
    elev2.SetInputConnection(src2.GetOutputPort())
    elev2.SetLowPoint(0, 0, 0)
    elev2.SetHighPoint(4, 4, 4)
    elev2.Update()

    mb.SetBlock(0, elev1.GetOutput())
    mb.SetBlock(1, elev2.GetOutput())
    return mb


def make_sphere():
    """Create a vtkPolyData sphere."""
    src = vtkSphereSource()
    src.SetThetaResolution(10)
    src.SetPhiResolution(10)
    src.Update()
    return src.GetOutput()


# -------------------------------------------------------------------
# 1. Single-array reductions
# -------------------------------------------------------------------
def test_single_array_reductions():
    print("Testing single-array reductions...")
    ds = make_image_dataset()
    elev = ds.point_data["Elevation"]
    elev_np = np.asarray(elev).astype(np.float64)

    # sum
    check_close(functions.sum(elev), np.sum(elev_np), "sum()")
    check_close(functions.sum(elev, axis=0), np.sum(elev_np, axis=0), "sum(axis=0)")

    # max / min
    check_close(functions.max(elev), np.max(elev_np), "max()")
    check_close(functions.min(elev), np.min(elev_np), "min()")
    check_close(functions.max(elev, axis=0), np.max(elev_np, axis=0), "max(axis=0)")
    check_close(functions.min(elev, axis=0), np.min(elev_np, axis=0), "min(axis=0)")

    # mean / var / std
    check_close(functions.mean(elev), np.mean(elev_np), "mean()")
    check_close(functions.var(elev), np.var(elev_np), "var()", rtol=1e-4)
    check_close(functions.std(elev), np.std(elev_np), "std()", rtol=1e-4)

    # all
    result = functions.all(elev >= 0)
    check(bool(result), "all(elev >= 0) should be True")

    result2 = functions.all(elev > 0.5)
    # Not all elevation values are > 0.5
    check(not bool(result2), "all(elev > 0.5) should be False")

    # NoneArray → NoneArray
    check(functions.sum(NoneArray) is NoneArray, "sum(NoneArray) → NoneArray")
    check(functions.max(NoneArray) is NoneArray, "max(NoneArray) → NoneArray")
    check(functions.min(NoneArray) is NoneArray, "min(NoneArray) → NoneArray")

    print("  single-array reductions OK")


# -------------------------------------------------------------------
# 2. Composite-array reductions
# -------------------------------------------------------------------
def test_composite_reductions():
    print("Testing composite-array reductions...")
    mb = make_multiblock()
    comp_elev = mb.point_data["Elevation"]
    check(isinstance(comp_elev, VTKPartitionedArray),
          "Composite point_data should return VTKPartitionedArray")

    # Collect per-block numpy arrays for reference
    blocks = [np.asarray(a).astype(np.float64) for a in comp_elev.arrays
              if a is not NoneArray]

    # sum
    expected_sum = sum(np.sum(b) for b in blocks)
    check_close(functions.sum(comp_elev), expected_sum, "composite sum()")

    # max / min
    expected_max = max(np.max(b) for b in blocks)
    expected_min = min(np.min(b) for b in blocks)
    check_close(functions.max(comp_elev), expected_max, "composite max()")
    check_close(functions.min(comp_elev), expected_min, "composite min()")

    # mean
    total_count = sum(b.size for b in blocks)
    expected_mean = expected_sum / total_count
    check_close(functions.mean(comp_elev), expected_mean, "composite mean()")

    # var / std
    expected_var = sum(np.sum((b - expected_mean)**2) for b in blocks) / total_count
    check_close(functions.var(comp_elev), expected_var, "composite var()", rtol=1e-4)
    check_close(functions.std(comp_elev), np.sqrt(expected_var),
                "composite std()", rtol=1e-4)

    # all
    result = functions.all(comp_elev >= 0)
    check(bool(result), "composite all(elev >= 0) should be True")

    # axis=1 on composite → per-block results (VTKPartitionedArray)
    # Use RTAnalytic which has 1-component scalars, so axis=1 is not applicable.
    # Instead test with a vector field: construct one from elevation
    block0_elev = comp_elev.arrays[0]
    block1_elev = comp_elev.arrays[1]

    # Composite with NoneArray block
    pa_with_none = VTKPartitionedArray([block0_elev, NoneArray])
    check_close(functions.sum(pa_with_none),
                np.sum(np.asarray(block0_elev).astype(np.float64)),
                "composite sum with NoneArray block")

    print("  composite reductions OK")


# -------------------------------------------------------------------
# 3. Vector/matrix operations
# -------------------------------------------------------------------
def test_vector_operations():
    print("Testing vector/matrix operations...")

    # Create a vector array
    n = 20
    vdata = np.random.RandomState(42).rand(n, 3).astype(np.float64)
    vtk_v = numpy_support.numpy_to_vtk(vdata)
    vtk_v.SetName("vectors")
    vtk_v._association = ArrayAssociation.POINT

    # cross(v, v) = zero
    c = functions.cross(vtk_v, vtk_v)
    check_close(np.asarray(c), np.zeros((n, 3)), "cross(v, v) = 0")

    # dot(v, v) = sum of squares per row
    d = functions.dot(vtk_v, vtk_v)
    expected_dot = np.sum(vdata * vdata, axis=1)
    check_close(np.asarray(d), expected_dot, "dot(v, v)")

    # mag(v) = sqrt(dot(v, v))
    m = functions.mag(vtk_v)
    check_close(np.asarray(m), np.sqrt(expected_dot), "mag(v)")

    # norm(v) = unit vectors
    normed = functions.norm(vtk_v)
    mags = np.sqrt(np.sum(np.asarray(normed)**2, axis=1))
    check_close(mags, np.ones(n), "norm(v) magnitudes ≈ 1")

    # make_vector
    x = numpy_support.numpy_to_vtk(vdata[:, 0].copy())
    y = numpy_support.numpy_to_vtk(vdata[:, 1].copy())
    z = numpy_support.numpy_to_vtk(vdata[:, 2].copy())
    mv = functions.make_vector(x, y, z)
    check(np.asarray(mv).shape == (n, 3), f"make_vector shape: {np.asarray(mv).shape}")
    check_close(np.asarray(mv), vdata, "make_vector values")

    # make_vector with 2 args (z defaults to 0)
    mv2 = functions.make_vector(x, y)
    expected_2d = np.column_stack([vdata[:, 0], vdata[:, 1], np.zeros(n)])
    check_close(np.asarray(mv2), expected_2d, "make_vector 2-arg")

    # matmul: matrix × vector
    matrices = np.eye(3, dtype=np.float64)[np.newaxis, :, :].repeat(n, axis=0)
    mat_vtk = numpy_support.numpy_to_vtk(matrices.reshape(n, 9))
    mat_vtk._association = ArrayAssociation.POINT
    # Reshape for matmul: needs 3D
    # matmul expects 2D or 3D arrays; test vector × vector (2D × 2D)
    result = functions.matmul(vtk_v, vtk_v)
    expected_mm = np.einsum('...j,...j', vdata, vdata)
    check_close(np.asarray(result), expected_mm, "matmul vector·vector")

    # trace on identity matrices
    identity = np.eye(3, dtype=np.float64)[np.newaxis, :, :].repeat(5, axis=0)
    id_flat = numpy_support.numpy_to_vtk(identity.reshape(5, 9))
    id_flat._association = ArrayAssociation.POINT
    # trace needs 3D input
    id_3d = np.asarray(id_flat).reshape(5, 3, 3)
    id_vtk_3d = numpy_support.numpy_to_vtk(id_3d.reshape(5, 9))
    id_vtk_3d._association = ArrayAssociation.POINT
    # _trace works on 3D arrays
    tr = functions._trace(id_3d)
    check_close(np.asarray(tr), np.full(5, 3.0), "trace of identity = 3")

    # Composite dispatch for cross, dot, mag
    pa = VTKPartitionedArray([vtk_v, vtk_v])
    c_comp = functions.cross(pa, pa)
    check(isinstance(c_comp, VTKPartitionedArray), "composite cross returns VTKPartitionedArray")
    check_close(np.asarray(c_comp.arrays[0]), np.zeros((n, 3)), "composite cross block 0")

    d_comp = functions.dot(pa, pa)
    check(isinstance(d_comp, VTKPartitionedArray), "composite dot returns VTKPartitionedArray")

    m_comp = functions.mag(pa)
    check(isinstance(m_comp, VTKPartitionedArray), "composite mag returns VTKPartitionedArray")

    print("  vector/matrix operations OK")


# -------------------------------------------------------------------
# 4. Metadata propagation
# -------------------------------------------------------------------
def test_metadata_propagation():
    print("Testing metadata propagation...")
    ds = make_image_dataset()
    elev = ds.point_data["Elevation"]

    # Verify source metadata
    check(hasattr(elev, '_association'), "Elevation has _association")
    check(elev._association == ArrayAssociation.POINT,
          f"Elevation association: {elev._association}")
    check(elev.dataset is ds, "Elevation dataset is the image data")

    # _to_vtk_array preserves metadata
    result = np.asarray(elev) * 2.0
    wrapped = functions._to_vtk_array(result, elev)
    check(hasattr(wrapped, '_association'), "_to_vtk_array result has _association")
    check(wrapped._association == ArrayAssociation.POINT,
          "_to_vtk_array preserves association")

    # cross/dot/mag carry metadata
    n = ds.GetNumberOfPoints()
    vdata = np.random.RandomState(99).rand(n, 3).astype(np.float64)
    vtk_v = numpy_support.numpy_to_vtk(vdata)
    vtk_v.SetName("test_vec")
    vtk_v._association = ArrayAssociation.POINT
    vtk_v._set_dataset(ds)

    d = functions.dot(vtk_v, vtk_v)
    check(hasattr(d, '_association'), "dot result has _association")
    check(d._association == ArrayAssociation.POINT, "dot preserves association")

    m = functions.mag(vtk_v)
    check(hasattr(m, '_association'), "mag result has _association")

    # make_vector propagates metadata
    x_arr = numpy_support.numpy_to_vtk(vdata[:, 0].copy())
    x_arr._association = ArrayAssociation.POINT
    x_arr._set_dataset(ds)
    y_arr = numpy_support.numpy_to_vtk(vdata[:, 1].copy())
    y_arr._association = ArrayAssociation.POINT
    y_arr._set_dataset(ds)

    mv = functions.make_vector(x_arr, y_arr)
    check(hasattr(mv, '_association'), "make_vector result has _association")
    check(mv._association == ArrayAssociation.POINT, "make_vector preserves association")

    print("  metadata propagation OK")


# -------------------------------------------------------------------
# 5. Composite dispatch utilities
# -------------------------------------------------------------------
def test_composite_dispatch():
    print("Testing composite dispatch utilities...")

    # apply_ufunc on VTKPartitionedArray
    a1 = numpy_support.numpy_to_vtk(np.array([1.0, 2.0, 3.0]))
    a2 = numpy_support.numpy_to_vtk(np.array([4.0, 5.0, 6.0]))
    pa = VTKPartitionedArray([a1, a2])

    def double_it(arr):
        return numpy_support.numpy_to_vtk(np.asarray(arr) * 2)

    result = functions.apply_ufunc(double_it, pa)
    check(isinstance(result, VTKPartitionedArray),
          "apply_ufunc on composite returns VTKPartitionedArray")
    check_close(np.asarray(result.arrays[0]), np.array([2.0, 4.0, 6.0]),
                "apply_ufunc block 0")
    check_close(np.asarray(result.arrays[1]), np.array([8.0, 10.0, 12.0]),
                "apply_ufunc block 1")

    # apply_ufunc on NoneArray
    check(functions.apply_ufunc(double_it, NoneArray) is NoneArray,
          "apply_ufunc(NoneArray) → NoneArray")

    # apply_ufunc on plain array
    plain = np.array([1.0, 2.0])
    result_plain = functions.apply_ufunc(lambda x: x * 3, plain)
    check_close(result_plain, np.array([3.0, 6.0]), "apply_ufunc on plain array")

    # apply_dfunc: two composites
    result_df = functions.apply_dfunc(np.add, pa, pa)
    check(isinstance(result_df, VTKPartitionedArray),
          "apply_dfunc(comp, comp) returns VTKPartitionedArray")
    check_close(np.asarray(result_df.arrays[0]), np.array([2.0, 4.0, 6.0]),
                "apply_dfunc(comp, comp) block 0")

    # apply_dfunc: composite + scalar
    result_ds = functions.apply_dfunc(np.add, pa, np.float64(10.0))
    check(isinstance(result_ds, VTKPartitionedArray),
          "apply_dfunc(comp, scalar) returns VTKPartitionedArray")
    check_close(np.asarray(result_ds.arrays[0]), np.array([11.0, 12.0, 13.0]),
                "apply_dfunc(comp, scalar) block 0")

    # apply_dfunc: NoneArray
    check(functions.apply_dfunc(np.add, NoneArray, np.float64(1.0)) is NoneArray,
          "apply_dfunc(NoneArray, scalar) → NoneArray")

    # _make_dsfunc
    def _dummy_ds_func(array, ds):
        return numpy_support.numpy_to_vtk(np.asarray(array) + 1.0)

    dsfunc = functions._make_dsfunc(_dummy_ds_func)
    # on plain array
    plain_arr = numpy_support.numpy_to_vtk(np.array([1.0, 2.0, 3.0]))
    result_dsf = dsfunc(plain_arr, ds=None)
    check_close(np.asarray(result_dsf), np.array([2.0, 3.0, 4.0]),
                "_make_dsfunc on plain array")

    # on NoneArray
    check(dsfunc(NoneArray) is NoneArray, "_make_dsfunc(NoneArray) → NoneArray")

    print("  composite dispatch utilities OK")


# -------------------------------------------------------------------
# 6. VTK filter wrappers — smoke tests
# -------------------------------------------------------------------
def test_vtk_filter_smoke():
    print("Testing VTK filter wrappers (smoke tests)...")
    ds = make_image_dataset()
    elev = ds.point_data["Elevation"]
    npts = ds.GetNumberOfPoints()

    # gradient of scalar → shape (n, 3)
    grad = functions.gradient(elev, ds)
    grad_np = np.asarray(grad)
    check(grad_np.shape[1] == 3,
          f"gradient shape: {grad_np.shape}, expected (*, 3)")

    # surface_normal on polydata → shape (ncells, 3)
    sphere = make_sphere()
    normals = functions.surface_normal(sphere)
    normals_np = np.asarray(normals)
    check(normals_np.shape[1] == 3,
          f"surface_normal shape: {normals_np.shape}, expected (*, 3)")

    print("  VTK filter smoke tests OK")


# -------------------------------------------------------------------
# 7. NoneArray handling in private impls
# -------------------------------------------------------------------
def test_none_array_handling():
    print("Testing NoneArray handling...")

    # Reductions
    check(functions._sum(NoneArray) is NoneArray, "_sum(NoneArray)")
    check(functions._max(NoneArray) is NoneArray, "_max(NoneArray)")
    check(functions._min(NoneArray) is NoneArray, "_min(NoneArray)")
    check(functions._all(NoneArray) is NoneArray, "_all(NoneArray)")
    check(functions._mean(NoneArray) is NoneArray, "_mean(NoneArray)")
    check(functions._var(NoneArray) is NoneArray, "_var(NoneArray)")

    # Binary ops
    v = numpy_support.numpy_to_vtk(np.array([[1.0, 2.0, 3.0]]))
    check(functions._cross(NoneArray, v) is NoneArray, "_cross(NoneArray, v)")
    check(functions._cross(v, NoneArray) is NoneArray, "_cross(v, NoneArray)")
    check(functions._dot(NoneArray, v) is NoneArray, "_dot(NoneArray, v)")
    check(functions._dot(v, NoneArray) is NoneArray, "_dot(v, NoneArray)")

    # mag
    check(functions._mag(NoneArray) is NoneArray, "_mag(NoneArray)")

    # make_vector
    s = numpy_support.numpy_to_vtk(np.array([1.0]))
    check(functions._make_vector(NoneArray, s) is NoneArray,
          "_make_vector(NoneArray, y)")
    check(functions._make_vector(s, NoneArray) is NoneArray,
          "_make_vector(x, NoneArray)")

    print("  NoneArray handling OK")


# -------------------------------------------------------------------
# 8. det/eigenvalue on 3x3 matrices via VTK filter
# -------------------------------------------------------------------
def test_matrix_operations():
    print("Testing matrix operations (det, eigenvalue, trace)...")

    # Create array of 3x3 identity matrices
    n = 4
    identity = np.eye(3, dtype=np.float64)[np.newaxis, :, :].repeat(n, axis=0)

    # det of identity = 1
    det_result = functions._det(identity)
    det_np = np.asarray(det_result)
    check_close(det_np, np.ones(n), "det(identity) = 1")

    # eigenvalue of identity = [1, 1, 1]
    eig_result = functions._eigenvalue(identity)
    eig_np = np.asarray(eig_result)
    check(eig_np.shape == (n, 3), f"eigenvalue shape: {eig_np.shape}")
    check_close(eig_np, np.ones((n, 3)), "eigenvalue(identity) = [1,1,1]")

    # trace of identity = 3
    tr = functions._trace(identity)
    check_close(np.asarray(tr), np.full(n, 3.0), "trace(identity) = 3")

    print("  matrix operations OK")


# -------------------------------------------------------------------
# Run all tests
# -------------------------------------------------------------------
test_single_array_reductions()
test_composite_reductions()
test_vector_operations()
test_metadata_propagation()
test_composite_dispatch()
test_vtk_filter_smoke()
test_none_array_handling()
test_matrix_operations()

if errors:
    print(f"\n{errors} error(s) found.")
    sys.exit(1)
else:
    print("\nAll tests passed.")
