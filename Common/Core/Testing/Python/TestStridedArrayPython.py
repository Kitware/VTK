# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
"""Tests for VTKStridedArray numpy-compatible mixin."""

import sys

import numpy
from numpy.testing import assert_array_almost_equal, assert_array_equal

from vtkmodules.vtkCommonCore import (
    vtkAOSDataArrayTemplate,
    vtkStridedArray,
)
from vtkmodules.util.numpy_support import numpy_to_vtk
from vtkmodules.numpy_interface.vtk_strided_array import VTKStridedArray

errors = 0


def check(condition, msg):
    global errors
    if not condition:
        print("ERROR:", msg)
        errors += 1


def make_strided(buffer_values, ncomps, stride, offset=0):
    """Create a vtkStridedArray from buffer values using the vtkBuffer overload.

    Parameters
    ----------
    buffer_values : list of float
        Raw buffer data.
    ncomps : int
        Number of components per tuple.
    stride : int
        Stride between tuple starts in the buffer.
    offset : int
        Offset into the buffer for the first value.
    """
    buf_np = numpy.array(buffer_values, dtype=numpy.float64)
    buf_vtk = numpy_to_vtk(buf_np)
    # Pass the vtkBuffer (via GetBuffer()) — C++ stores it with ref counting,
    # keeping the memory alive.
    vtk_buffer = buf_vtk.GetBuffer()

    arr = vtkStridedArray['float64']()
    arr.ConstructBackend(vtk_buffer, stride, ncomps, offset)
    return arr


def test_override_detection():
    """Test that strided arrays get the VTKStridedArray mixin."""
    # Buffer: [10, 20, 30, 40, 50], stride=1, 1-comp, 5 tuples
    arr = make_strided([10.0, 20.0, 30.0, 40.0, 50.0], 1, 1)
    check(isinstance(arr, VTKStridedArray),
          f"Expected VTKStridedArray, got {type(arr)}")


def test_properties():
    """Test shape, dtype, ndim, size, len, stride, offset."""
    arr = make_strided([10.0, 20.0, 30.0, 40.0, 50.0], 1, 1)

    check(arr.shape == (5,), f"Expected shape (5,), got {arr.shape}")
    check(arr.dtype == numpy.float64, f"Expected float64, got {arr.dtype}")
    check(arr.ndim == 1, f"Expected ndim 1, got {arr.ndim}")
    check(arr.size == 5, f"Expected size 5, got {arr.size}")
    check(len(arr) == 5, f"Expected len 5, got {len(arr)}")
    check(arr.stride == 1, f"Expected stride 1, got {arr.stride}")
    check(arr.offset == 0, f"Expected offset 0, got {arr.offset}")


def test_materialization():
    """Test np.array(strided) produces correct values."""
    # Buffer: [10, 20, 30, 40, 50], stride=1, all values
    arr = make_strided([10.0, 20.0, 30.0, 40.0, 50.0], 1, 1)
    result = numpy.array(arr)
    expected = numpy.array([10.0, 20.0, 30.0, 40.0, 50.0])
    assert_array_equal(result, expected)


def test_to_numpy():
    """Test to_numpy() method."""
    arr = make_strided([10.0, 20.0, 30.0, 40.0, 50.0], 1, 1)
    result = arr.to_numpy()
    expected = numpy.array([10.0, 20.0, 30.0, 40.0, 50.0])
    assert_array_equal(result, expected)


def test_scalar_arithmetic():
    """Test arithmetic with scalars."""
    arr = make_strided([10.0, 20.0, 30.0], 1, 1)

    result = numpy.asarray(arr + 100)
    expected = numpy.array([110.0, 120.0, 130.0])
    assert_array_equal(result, expected)

    result = numpy.asarray(arr * 2)
    expected = numpy.array([20.0, 40.0, 60.0])
    assert_array_equal(result, expected)


def test_array_arithmetic():
    """Test arithmetic with numpy arrays."""
    arr = make_strided([10.0, 20.0, 30.0], 1, 1)
    np_arr = numpy.array([1.0, 2.0, 3.0])

    result = numpy.asarray(arr + np_arr)
    expected = numpy.array([11.0, 22.0, 33.0])
    assert_array_equal(result, expected)


def test_reductions():
    """Test sum, min, max, mean reductions."""
    arr = make_strided([10.0, 20.0, 30.0, 40.0, 50.0], 1, 1)

    check(numpy.sum(arr) == 150.0,
          f"Expected sum 150.0, got {numpy.sum(arr)}")
    check(numpy.min(arr) == 10.0,
          f"Expected min 10.0, got {numpy.min(arr)}")
    check(numpy.max(arr) == 50.0,
          f"Expected max 50.0, got {numpy.max(arr)}")
    check(numpy.mean(arr) == 30.0,
          f"Expected mean 30.0, got {numpy.mean(arr)}")


def test_reduction_methods():
    """Test .sum(), .min(), .max(), .mean() methods."""
    arr = make_strided([10.0, 20.0, 30.0, 40.0, 50.0], 1, 1)

    check(arr.sum() == 150.0, f"Expected sum 150.0, got {arr.sum()}")
    check(arr.min() == 10.0, f"Expected min 10.0, got {arr.min()}")
    check(arr.max() == 50.0, f"Expected max 50.0, got {arr.max()}")
    check(arr.mean() == 30.0, f"Expected mean 30.0, got {arr.mean()}")


def test_ufuncs():
    """Test numpy ufuncs (sin, sqrt)."""
    arr = make_strided([1.0, 4.0, 9.0, 16.0], 1, 1)

    result = numpy.asarray(numpy.sqrt(arr))
    expected = numpy.sqrt(numpy.array([1.0, 4.0, 9.0, 16.0]))
    assert_array_almost_equal(result, expected)

    result = numpy.asarray(numpy.sin(arr))
    expected = numpy.sin(numpy.array([1.0, 4.0, 9.0, 16.0]))
    assert_array_almost_equal(result, expected)


def test_scalar_indexing():
    """Test element indexing without materialization."""
    arr = make_strided([10.0, 20.0, 30.0, 40.0, 50.0], 1, 1)

    check(arr[0] == 10.0, f"Expected arr[0]=10.0, got {arr[0]}")
    check(arr[2] == 30.0, f"Expected arr[2]=30.0, got {arr[2]}")
    check(arr[-1] == 50.0, f"Expected arr[-1]=50.0, got {arr[-1]}")


def test_slice_indexing():
    """Test slice indexing (materializes)."""
    arr = make_strided([10.0, 20.0, 30.0, 40.0, 50.0], 1, 1)

    result = arr[1:4]
    expected = numpy.array([20.0, 30.0, 40.0])
    assert_array_equal(result, expected)


def test_fancy_indexing():
    """Test fancy integer indexing (materializes)."""
    arr = make_strided([10.0, 20.0, 30.0, 40.0, 50.0], 1, 1)

    result = arr[[0, 2, 4]]
    expected = numpy.array([10.0, 30.0, 50.0])
    assert_array_equal(result, expected)


def test_multicomponent():
    """Test multi-component strided arrays.

    Buffer: [x0,y0,z0, x1,y1,z1, x2,y2,z2]
    stride=3, ncomps=3 -> 3 tuples of 3 components each
    """
    buffer = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0]
    arr = make_strided(buffer, 3, 3)

    check(arr.shape == (3, 3), f"Expected shape (3, 3), got {arr.shape}")

    result = numpy.array(arr)
    expected = numpy.array([[1.0, 2.0, 3.0], [4.0, 5.0, 6.0], [7.0, 8.0, 9.0]])
    assert_array_equal(result, expected)

    # Scalar indexing for multi-component
    row = arr[1]
    assert_array_equal(row, numpy.array([4.0, 5.0, 6.0]))


def test_readonly():
    """Test that __setitem__ raises TypeError."""
    arr = make_strided([10.0, 20.0, 30.0], 1, 1)

    try:
        arr[0] = 99.0
        check(False, "__setitem__ should raise TypeError")
    except TypeError:
        pass


def test_repr():
    """Test __repr__ output."""
    arr = make_strided([10.0, 20.0, 30.0], 1, 1)

    r = repr(arr)
    check("VTKStridedArray" in r, f"Expected VTKStridedArray in repr, got {r}")
    check("stride=" in r, f"Expected stride= in repr, got {r}")
    check("offset=" in r, f"Expected offset= in repr, got {r}")


def test_comparison_operators():
    """Test comparison operators."""
    arr = make_strided([10.0, 20.0, 30.0, 40.0, 50.0], 1, 1)

    result = numpy.asarray(arr > 25)
    expected = numpy.array([False, False, True, True, True])
    assert_array_equal(result, expected)


def test_iter():
    """Test iteration."""
    arr = make_strided([10.0, 20.0, 30.0], 1, 1)

    vals = list(arr)
    check(len(vals) == 3, f"Expected 3 values, got {len(vals)}")
    check(vals[0] == 10.0, f"Expected first value 10.0, got {vals[0]}")
    check(vals[2] == 30.0, f"Expected third value 30.0, got {vals[2]}")


def test_stride_greater_than_ncomps():
    """Test stride > ncomponents (interleaved with extra data).

    Buffer: [x0,y0,EXTRA, x1,y1,EXTRA, x2,y2,EXTRA]
    stride=3, ncomps=2, offset=0 -> extract (x,y) pairs
    """
    buffer = [1.0, 2.0, 999.0, 4.0, 5.0, 999.0, 7.0, 8.0, 999.0]
    arr = make_strided(buffer, 2, 3, offset=0)

    check(arr.shape == (3, 2), f"Expected shape (3, 2), got {arr.shape}")
    check(arr.stride == 3, f"Expected stride 3, got {arr.stride}")

    result = numpy.array(arr)
    expected = numpy.array([[1.0, 2.0], [4.0, 5.0], [7.0, 8.0]])
    assert_array_equal(result, expected)


def test_offset_greater_than_zero():
    """Test offset > 0 (skip leading values).

    Buffer: [EXTRA, x0,y0, EXTRA, x1,y1, EXTRA, x2,y2]
    stride=3, ncomps=2, offset=1 -> extract (x,y) starting at offset 1
    """
    buffer = [999.0, 1.0, 2.0, 999.0, 4.0, 5.0, 999.0, 7.0, 8.0]
    arr = make_strided(buffer, 2, 3, offset=1)

    check(arr.offset == 1, f"Expected offset 1, got {arr.offset}")

    result = numpy.array(arr)
    expected = numpy.array([[1.0, 2.0], [4.0, 5.0], [7.0, 8.0]])
    assert_array_equal(result, expected)


def test_single_component_with_stride():
    """Test single-component array with stride > 1.

    Buffer: [x0, EXTRA, x1, EXTRA, x2, EXTRA]
    stride=2, ncomps=1 -> extract every other value
    """
    buffer = [10.0, 999.0, 20.0, 999.0, 30.0, 999.0]
    arr = make_strided(buffer, 1, 2, offset=0)

    check(arr.shape == (3,), f"Expected shape (3,), got {arr.shape}")
    check(arr.stride == 2, f"Expected stride 2, got {arr.stride}")

    result = numpy.array(arr)
    expected = numpy.array([10.0, 20.0, 30.0])
    assert_array_equal(result, expected)


def test_nbytes():
    """Test nbytes property."""
    arr = make_strided([10.0, 20.0, 30.0], 1, 1)
    check(arr.nbytes == 3 * 8, f"Expected nbytes 24, got {arr.nbytes}")


def test_zero_copy():
    """Test that __array__ returns a zero-copy strided view.

    The numpy array returned by np.asarray(strided) should share memory
    with the original buffer — no DeepCopy.
    """
    buf_np = numpy.array([10.0, 999.0, 20.0, 999.0, 30.0, 999.0],
                         dtype=numpy.float64)
    buf_vtk = numpy_to_vtk(buf_np)
    vtk_buffer = buf_vtk.GetBuffer()

    arr = vtkStridedArray['float64']()
    arr.ConstructBackend(vtk_buffer, 2, 1, 0)

    view = numpy.asarray(arr)
    assert_array_equal(view, numpy.array([10.0, 20.0, 30.0]))

    # Verify zero-copy: the view should share memory with buf_np
    check(numpy.shares_memory(view, buf_np),
          "Expected zero-copy: view should share memory with buffer")


def test_zero_copy_multicomponent():
    """Test zero-copy strided view with multi-component + offset."""
    # Buffer: [EXTRA, x0,y0, EXTRA, x1,y1, EXTRA, x2,y2]
    buf_np = numpy.array([999.0, 1.0, 2.0, 999.0, 4.0, 5.0, 999.0, 7.0, 8.0],
                         dtype=numpy.float64)
    buf_vtk = numpy_to_vtk(buf_np)
    vtk_buffer = buf_vtk.GetBuffer()

    arr = vtkStridedArray['float64']()
    arr.ConstructBackend(vtk_buffer, 3, 2, 1)

    view = numpy.asarray(arr)
    expected = numpy.array([[1.0, 2.0], [4.0, 5.0], [7.0, 8.0]])
    assert_array_equal(view, expected)

    check(numpy.shares_memory(view, buf_np),
          "Expected zero-copy: multi-component view should share memory")


# ---- Run all tests ----

test_override_detection()
test_properties()
test_materialization()
test_to_numpy()
test_scalar_arithmetic()
test_array_arithmetic()
test_reductions()
test_reduction_methods()
test_ufuncs()
test_scalar_indexing()
test_slice_indexing()
test_fancy_indexing()
test_multicomponent()
test_readonly()
test_repr()
test_comparison_operators()
test_iter()
test_stride_greater_than_ncomps()
test_offset_greater_than_zero()
test_single_component_with_stride()
test_nbytes()
test_zero_copy()
test_zero_copy_multicomponent()

if errors:
    print(f"\n{errors} error(s) found!")
    sys.exit(1)
else:
    print("All tests passed.")
