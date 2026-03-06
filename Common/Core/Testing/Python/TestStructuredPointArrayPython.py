"""Test VTKStructuredPointArray numpy-compatible wrapper."""
import sys
import numpy as np
from vtkmodules.vtkCommonDataModel import vtkImageData, vtkRectilinearGrid
from vtkmodules.vtkCommonCore import vtkFloatArray
from vtkmodules.numpy_interface.vtk_structured_point_array import (
    VTKStructuredPointArray,
)


def test_image_data_auto_detection():
    """Test that accessing vtkImageData.points returns VTKStructuredPointArray."""
    img = vtkImageData()
    img.SetDimensions(3, 4, 5)
    img.SetOrigin(1.0, 2.0, 3.0)
    img.SetSpacing(0.5, 1.0, 1.5)
    points = img.points
    assert isinstance(points, VTKStructuredPointArray), \
        f"Expected VTKStructuredPointArray, got {type(points)}"
    return True


def test_rectilinear_grid():
    """Test VTKStructuredPointArray with vtkRectilinearGrid."""
    rg = vtkRectilinearGrid()
    rg.SetDimensions(3, 2, 2)

    xcoords = vtkFloatArray()
    for v in [0.0, 1.0, 3.0]:
        xcoords.InsertNextValue(v)

    ycoords = vtkFloatArray()
    for v in [0.0, 2.0]:
        ycoords.InsertNextValue(v)

    zcoords = vtkFloatArray()
    for v in [0.0, 5.0]:
        zcoords.InsertNextValue(v)

    rg.SetXCoordinates(xcoords)
    rg.SetYCoordinates(ycoords)
    rg.SetZCoordinates(zcoords)

    points = rg.points

    assert isinstance(points, VTKStructuredPointArray), \
        f"Expected VTKStructuredPointArray, got {type(points)}"

    # Verify first point
    pt = points[0]
    np.testing.assert_array_almost_equal(pt, [0.0, 0.0, 0.0])

    return True


def test_shape_len_dtype():
    """Test shape, len, and dtype properties."""
    img = vtkImageData()
    img.SetDimensions(3, 4, 5)
    img.SetOrigin(0.0, 0.0, 0.0)
    img.SetSpacing(1.0, 1.0, 1.0)
    points = img.points

    assert points.shape == (3 * 4 * 5, 3), f"shape mismatch: {points.shape}"
    assert len(points) == 60, f"len mismatch: {len(points)}"
    assert points.ndim == 2
    assert points.size == 60 * 3
    return True


def test_materialization():
    """Test np.array(wrapped) materialization matches expected meshgrid output."""
    img = vtkImageData()
    img.SetDimensions(3, 4, 2)
    img.SetOrigin(0.0, 0.0, 0.0)
    img.SetSpacing(1.0, 1.0, 1.0)
    points = img.points

    materialized = np.array(points)
    assert materialized.shape == (3 * 4 * 2, 3), f"shape: {materialized.shape}"

    # Build expected using meshgrid with Fortran ordering
    X = np.arange(3, dtype=materialized.dtype)
    Y = np.arange(4, dtype=materialized.dtype)
    Z = np.arange(2, dtype=materialized.dtype)
    gx, gy, gz = np.meshgrid(X, Y, Z, indexing='ij')
    expected = np.column_stack([
        gx.ravel(order='F'),
        gy.ravel(order='F'),
        gz.ravel(order='F')
    ])

    np.testing.assert_array_almost_equal(materialized, expected)
    return True


def test_single_point_indexing():
    """Test single-point indexing: points[0], points[-1], points[i]."""
    img = vtkImageData()
    img.SetDimensions(3, 4, 5)
    img.SetOrigin(1.0, 2.0, 3.0)
    img.SetSpacing(0.5, 1.0, 1.5)
    points = img.points

    materialized = np.array(points)

    np.testing.assert_array_almost_equal(points[0], materialized[0])
    np.testing.assert_array_almost_equal(points[-1], materialized[-1])
    np.testing.assert_array_almost_equal(points[13], materialized[13])
    return True


def test_slice_indexing():
    """Test slice indexing falls back correctly."""
    img = vtkImageData()
    img.SetDimensions(3, 4, 5)
    img.SetOrigin(0.0, 0.0, 0.0)
    img.SetSpacing(1.0, 1.0, 1.0)
    points = img.points

    materialized = np.array(points)
    sliced = points[0:5]
    np.testing.assert_array_almost_equal(sliced, materialized[0:5])
    return True


def test_ufunc_stays_lazy():
    """Test ufuncs stay lazy: np.sqrt(wrapped) returns VTKStructuredPointArray."""
    img = vtkImageData()
    img.SetDimensions(3, 4, 5)
    img.SetOrigin(1.0, 2.0, 3.0)
    img.SetSpacing(1.0, 1.0, 1.0)
    points = img.points

    result = np.sqrt(points)
    assert isinstance(result, VTKStructuredPointArray), \
        f"Expected VTKStructuredPointArray, got {type(result)}"

    materialized = np.array(points)
    np.testing.assert_array_almost_equal(np.array(result), np.sqrt(materialized))
    return True


def test_scalar_arithmetic_stays_lazy():
    """Test scalar arithmetic stays lazy: wrapped + 10, wrapped * 2."""
    img = vtkImageData()
    img.SetDimensions(3, 4, 5)
    img.SetOrigin(1.0, 2.0, 3.0)
    img.SetSpacing(1.0, 1.0, 1.0)
    points = img.points

    result_add = points + 10
    assert isinstance(result_add, VTKStructuredPointArray), \
        f"Expected VTKStructuredPointArray for +, got {type(result_add)}"

    result_mul = points * 2
    assert isinstance(result_mul, VTKStructuredPointArray), \
        f"Expected VTKStructuredPointArray for *, got {type(result_mul)}"

    materialized = np.array(points)
    np.testing.assert_array_almost_equal(np.array(result_add), materialized + 10)
    np.testing.assert_array_almost_equal(np.array(result_mul), materialized * 2)
    return True


def test_reductions():
    """Test reductions: np.sum, np.min, np.max with axis=0 and axis=None."""
    img = vtkImageData()
    img.SetDimensions(3, 4, 5)
    img.SetOrigin(1.0, 2.0, 3.0)
    img.SetSpacing(0.5, 1.0, 1.5)
    points = img.points

    materialized = np.array(points)

    # axis=0 reductions
    np.testing.assert_array_almost_equal(
        np.min(points, axis=0), np.min(materialized, axis=0))
    np.testing.assert_array_almost_equal(
        np.max(points, axis=0), np.max(materialized, axis=0))
    np.testing.assert_array_almost_equal(
        np.mean(points, axis=0), np.mean(materialized, axis=0))

    # scalar reductions
    np.testing.assert_almost_equal(
        np.sum(points), np.sum(materialized))
    np.testing.assert_almost_equal(
        np.min(points), np.min(materialized))
    np.testing.assert_almost_equal(
        np.max(points), np.max(materialized))
    return True


def test_ufunc_arithmetic_match_numpy():
    """Test that ufunc/arithmetic results match plain numpy."""
    img = vtkImageData()
    img.SetDimensions(4, 3, 2)
    img.SetOrigin(1.0, 2.0, 3.0)
    img.SetSpacing(0.5, 1.0, 1.5)
    points = img.points

    materialized = np.array(points)

    np.testing.assert_array_almost_equal(
        np.array(points + 5), materialized + 5)
    np.testing.assert_array_almost_equal(
        np.array(points - 1), materialized - 1)
    np.testing.assert_array_almost_equal(
        np.array(points * 3), materialized * 3)
    np.testing.assert_array_almost_equal(
        np.array(points / 2), materialized / 2)
    np.testing.assert_array_almost_equal(
        np.array(np.abs(points)), np.abs(materialized))
    np.testing.assert_array_almost_equal(
        np.array(-points), -materialized)
    return True


def test_column_indexing():
    """Test column indexing returns lazy VTKStructuredAxisArray."""
    from vtkmodules.numpy_interface.vtk_structured_point_array import (
        VTKStructuredAxisArray,
    )
    img = vtkImageData()
    img.SetDimensions(3, 4, 5)
    img.SetOrigin(1.0, 2.0, 3.0)
    img.SetSpacing(0.5, 1.0, 1.5)
    points = img.points

    x_col = points[:, 0]
    assert isinstance(x_col, VTKStructuredAxisArray), \
        f"Expected VTKStructuredAxisArray, got {type(x_col)}"

    materialized = np.array(points)
    np.testing.assert_array_almost_equal(
        np.array(x_col), materialized[:, 0])
    np.testing.assert_array_almost_equal(
        np.array(points[:, 1]), materialized[:, 1])
    np.testing.assert_array_almost_equal(
        np.array(points[:, 2]), materialized[:, 2])
    return True


def test_mixin_on_raw_array():
    """Test that the VTK class mixin is applied to vtkStructuredPointArray."""
    from vtkmodules.numpy_interface.vtk_structured_point_array import (
        VTKStructuredPointArrayMixin as Mixin,
    )
    img = vtkImageData()
    img.SetDimensions(3, 4, 5)
    img.SetOrigin(1.0, 2.0, 3.0)
    img.SetSpacing(0.5, 1.0, 1.5)

    arr = img.GetPoints().GetData()
    assert isinstance(arr, Mixin), \
        f"Expected VTKStructuredPointArray, got {type(arr)}"
    assert type(arr).__name__ == 'VTKStructuredPointArray', \
        f"Expected class name VTKStructuredPointArray, got {type(arr).__name__}"

    # numpy conversion
    nparr = np.asarray(arr)
    assert nparr.shape == (60, 3), f"shape mismatch: {nparr.shape}"
    np.testing.assert_array_almost_equal(arr[0], [1.0, 2.0, 3.0])

    # shape, dtype, ndim
    assert arr.shape == (60, 3)
    assert arr.dtype == np.float64
    assert arr.ndim == 2

    return True


def test_mixin_with_dataset():
    """Test mixin lazy operations via backend axis arrays."""
    from vtkmodules.numpy_interface.vtk_structured_point_array import (
        VTKStructuredPointArrayMixin as Mixin, VTKStructuredAxisArray,
        VTKStructuredPointArray,
    )
    img = vtkImageData()
    img.SetDimensions(3, 4, 5)
    img.SetOrigin(1.0, 2.0, 3.0)
    img.SetSpacing(0.5, 1.0, 1.5)

    arr = img.GetPoints().GetData()
    materialized = np.asarray(arr)

    # Column indexing returns lazy axis array
    x_col = arr[:, 0]
    assert isinstance(x_col, VTKStructuredAxisArray), \
        f"Expected VTKStructuredAxisArray, got {type(x_col)}"
    np.testing.assert_array_almost_equal(
        np.array(x_col), materialized[:, 0])

    # Ufunc stays lazy
    result = np.sqrt(arr)
    assert isinstance(result, VTKStructuredPointArray), \
        f"Expected VTKStructuredPointArray, got {type(result)}"
    np.testing.assert_array_almost_equal(
        np.array(result), np.sqrt(materialized))

    # Scalar arithmetic stays lazy
    result_add = arr + 10
    assert isinstance(result_add, VTKStructuredPointArray), \
        f"Expected VTKStructuredPointArray, got {type(result_add)}"
    np.testing.assert_array_almost_equal(
        np.array(result_add), materialized + 10)

    # Reductions use O(nx+ny+nz) formulas
    np.testing.assert_almost_equal(np.sum(arr), np.sum(materialized))
    np.testing.assert_array_almost_equal(
        np.min(arr, axis=0), np.min(materialized, axis=0))
    np.testing.assert_array_almost_equal(
        np.max(arr, axis=0), np.max(materialized, axis=0))
    np.testing.assert_array_almost_equal(
        np.mean(arr, axis=0), np.mean(materialized, axis=0))

    return True


# Run all tests
tests = [
    test_image_data_auto_detection,
    test_rectilinear_grid,
    test_shape_len_dtype,
    test_materialization,
    test_single_point_indexing,
    test_slice_indexing,
    test_ufunc_stays_lazy,
    test_scalar_arithmetic_stays_lazy,
    test_reductions,
    test_ufunc_arithmetic_match_numpy,
    test_column_indexing,
    test_mixin_on_raw_array,
    test_mixin_with_dataset,
]

failed = 0
for test in tests:
    try:
        test()
        print(f"PASSED: {test.__name__}")
    except Exception as e:
        print(f"FAILED: {test.__name__}: {e}")
        import traceback
        traceback.print_exc()
        failed += 1

if failed:
    print(f"\n{failed} test(s) failed")
    sys.exit(1)
else:
    print(f"\nAll {len(tests)} tests passed")
    sys.exit(0)
