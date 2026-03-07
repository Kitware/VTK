# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
"""Tests for VTKIndexedArray numpy-compatible mixin."""

import sys

import numpy
from numpy.testing import assert_array_almost_equal, assert_array_equal

from vtkmodules.vtkCommonCore import (
    vtkAOSDataArrayTemplate,
    vtkIndexedArray,
    vtkIdTypeArray,
)
from vtkmodules.numpy_interface.vtk_indexed_array import VTKIndexedArray

errors = 0


def check(condition, msg):
    global errors
    if not condition:
        print("ERROR:", msg)
        errors += 1


def make_aos_array(values, ncomps=1):
    """Create an AOS array from a list of values."""
    arr = vtkAOSDataArrayTemplate['float64']()
    arr.SetNumberOfComponents(ncomps)
    ntuples = len(values) // ncomps
    arr.SetNumberOfTuples(ntuples)
    for i, v in enumerate(values):
        arr.SetValue(i, v)
    return arr


def make_index_array(indexes):
    """Create a vtkIdTypeArray from a list of index values."""
    arr = vtkIdTypeArray()
    arr.SetNumberOfComponents(1)
    arr.SetNumberOfTuples(len(indexes))
    for i, v in enumerate(indexes):
        arr.SetValue(i, v)
    return arr


def make_indexed(base_values, indexes, ncomps=1):
    """Create a vtkIndexedArray from base values and index list.

    For multi-component arrays, indexes are value-level (flat):
    len(indexes) must be divisible by ncomps, and ntuples = len(indexes) // ncomps.
    """
    base = make_aos_array(base_values, ncomps)
    idx = make_index_array(indexes)
    indexed = vtkIndexedArray['float64']()
    indexed.SetNumberOfComponents(ncomps)
    indexed.SetNumberOfTuples(len(indexes) // ncomps)
    indexed.ConstructBackend(idx, base)
    return indexed


def test_override_detection():
    """Test that indexed arrays get the VTKIndexedArray mixin."""
    indexed = make_indexed([10.0, 20.0, 30.0, 40.0, 50.0], [4, 2, 0])
    check(isinstance(indexed, VTKIndexedArray),
          f"Expected VTKIndexedArray, got {type(indexed)}")


def test_accessor_properties():
    """Test base_array and index_array properties."""
    base = make_aos_array([10.0, 20.0, 30.0])
    idx = make_index_array([2, 0, 1])
    indexed = vtkIndexedArray['float64']()
    indexed.SetNumberOfComponents(1)
    indexed.SetNumberOfTuples(3)
    indexed.ConstructBackend(idx, base)

    check(indexed.base_array is not None, "base_array should not be None")
    check(indexed.index_array is not None, "index_array should not be None")
    check(indexed.base_array.GetNumberOfTuples() == 3,
          f"base_array should have 3 tuples, got {indexed.base_array.GetNumberOfTuples()}")
    check(indexed.index_array.GetNumberOfTuples() == 3,
          f"index_array should have 3 tuples, got {indexed.index_array.GetNumberOfTuples()}")


def test_properties():
    """Test shape, dtype, ndim, size, len."""
    indexed = make_indexed([10.0, 20.0, 30.0, 40.0, 50.0], [4, 2, 0])

    check(indexed.shape == (3,), f"Expected shape (3,), got {indexed.shape}")
    check(indexed.dtype == numpy.float64, f"Expected float64, got {indexed.dtype}")
    check(indexed.ndim == 1, f"Expected ndim 1, got {indexed.ndim}")
    check(indexed.size == 3, f"Expected size 3, got {indexed.size}")
    check(len(indexed) == 3, f"Expected len 3, got {len(indexed)}")


def test_materialization():
    """Test np.array(indexed) matches manual base[indexes]."""
    indexed = make_indexed([10.0, 20.0, 30.0, 40.0, 50.0], [4, 2, 0])

    result = numpy.array(indexed)
    expected = numpy.array([50.0, 30.0, 10.0])
    assert_array_equal(result, expected)


def test_to_numpy():
    """Test to_numpy() method."""
    indexed = make_indexed([10.0, 20.0, 30.0, 40.0, 50.0], [4, 2, 0])

    result = indexed.to_numpy()
    expected = numpy.array([50.0, 30.0, 10.0])
    assert_array_equal(result, expected)


def test_scalar_arithmetic():
    """Test arithmetic with scalars."""
    indexed = make_indexed([10.0, 20.0, 30.0, 40.0, 50.0], [4, 2, 0])

    result = numpy.asarray(indexed + 100)
    expected = numpy.array([150.0, 130.0, 110.0])
    assert_array_equal(result, expected)

    result = numpy.asarray(indexed * 2)
    expected = numpy.array([100.0, 60.0, 20.0])
    assert_array_equal(result, expected)


def test_indexed_numpy_arithmetic():
    """Test arithmetic between indexed array and numpy array."""
    indexed = make_indexed([10.0, 20.0, 30.0, 40.0, 50.0], [4, 2, 0])

    np_arr = numpy.array([1.0, 2.0, 3.0])
    result = numpy.asarray(indexed + np_arr)
    expected = numpy.array([51.0, 32.0, 13.0])
    assert_array_equal(result, expected)


def test_reductions():
    """Test sum, min, max, mean reductions."""
    indexed = make_indexed([10.0, 20.0, 30.0, 40.0, 50.0], [4, 2, 0])

    check(numpy.sum(indexed) == 90.0,
          f"Expected sum 90.0, got {numpy.sum(indexed)}")
    check(numpy.min(indexed) == 10.0,
          f"Expected min 10.0, got {numpy.min(indexed)}")
    check(numpy.max(indexed) == 50.0,
          f"Expected max 50.0, got {numpy.max(indexed)}")
    check(numpy.mean(indexed) == 30.0,
          f"Expected mean 30.0, got {numpy.mean(indexed)}")


def test_reduction_methods():
    """Test .sum(), .min(), .max(), .mean() methods."""
    indexed = make_indexed([10.0, 20.0, 30.0, 40.0, 50.0], [4, 2, 0])

    check(indexed.sum() == 90.0, f"Expected sum 90.0, got {indexed.sum()}")
    check(indexed.min() == 10.0, f"Expected min 10.0, got {indexed.min()}")
    check(indexed.max() == 50.0, f"Expected max 50.0, got {indexed.max()}")
    check(indexed.mean() == 30.0, f"Expected mean 30.0, got {indexed.mean()}")


def test_ufuncs():
    """Test numpy ufuncs (sin, sqrt)."""
    indexed = make_indexed([0.0, 1.0, 4.0, 9.0, 16.0], [2, 3, 4])

    result = numpy.asarray(numpy.sqrt(indexed))
    expected = numpy.sqrt(numpy.array([4.0, 9.0, 16.0]))
    assert_array_almost_equal(result, expected)

    result = numpy.asarray(numpy.sin(indexed))
    expected = numpy.sin(numpy.array([4.0, 9.0, 16.0]))
    assert_array_almost_equal(result, expected)


def test_scalar_indexing():
    """Test element indexing without materialization."""
    indexed = make_indexed([10.0, 20.0, 30.0, 40.0, 50.0], [4, 2, 0])

    check(indexed[0] == 50.0, f"Expected indexed[0]=50.0, got {indexed[0]}")
    check(indexed[1] == 30.0, f"Expected indexed[1]=30.0, got {indexed[1]}")
    check(indexed[2] == 10.0, f"Expected indexed[2]=10.0, got {indexed[2]}")
    check(indexed[-1] == 10.0, f"Expected indexed[-1]=10.0, got {indexed[-1]}")


def test_slice_indexing():
    """Test slice indexing (values correct, and stays lazy)."""
    indexed = make_indexed([10.0, 20.0, 30.0, 40.0, 50.0], [4, 3, 2, 1, 0])

    result = indexed[1:4]
    expected = numpy.array([40.0, 30.0, 20.0])
    assert_array_equal(numpy.asarray(result), expected)


def test_multicomponent():
    """Test multi-component indexed arrays.

    Indexes are value-level (flat): each index points to a value in the
    flattened base array.  With 2 components, every 2 consecutive indexed
    values form one tuple.
    """
    # Base: 3 tuples with 2 components each: values [1,2,3,4,5,6]
    base_values = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0]
    # Select values [4,5,0,1] -> tuples (5.0,6.0) and (1.0,2.0)
    indexes = [4, 5, 0, 1]

    indexed = make_indexed(base_values, indexes, ncomps=2)

    check(indexed.shape == (2, 2), f"Expected shape (2, 2), got {indexed.shape}")

    result = numpy.array(indexed)
    expected = numpy.array([[5.0, 6.0], [1.0, 2.0]])
    assert_array_equal(result, expected)

    # Scalar indexing for multi-component
    row = indexed[0]
    assert_array_equal(row, numpy.array([5.0, 6.0]))

    row = indexed[1]
    assert_array_equal(row, numpy.array([1.0, 2.0]))


def test_readonly():
    """Test that __setitem__ raises TypeError."""
    indexed = make_indexed([10.0, 20.0, 30.0], [0, 1])

    try:
        indexed[0] = 99.0
        check(False, "__setitem__ should raise TypeError")
    except TypeError:
        pass


def test_repr():
    """Test __repr__ output."""
    indexed = make_indexed([10.0, 20.0, 30.0], [2, 0])

    r = repr(indexed)
    check("VTKIndexedArray" in r, f"Expected VTKIndexedArray in repr, got {r}")
    check("shape=" in r, f"Expected shape= in repr, got {r}")


def test_comparison_operators():
    """Test comparison operators."""
    indexed = make_indexed([10.0, 20.0, 30.0, 40.0, 50.0], [0, 1, 2, 3, 4])

    result = numpy.asarray(indexed > 25)
    expected = numpy.array([False, False, True, True, True])
    assert_array_equal(result, expected)


def test_construction_from_python():
    """Test construction from Python with (indexes, array) args."""
    base = make_aos_array([100.0, 200.0, 300.0])
    idx = make_index_array([2, 0])

    indexed = vtkIndexedArray['float64'](idx, base)

    check(isinstance(indexed, VTKIndexedArray),
          f"Expected VTKIndexedArray, got {type(indexed)}")
    check(indexed.GetNumberOfTuples() == 2,
          f"Expected 2 tuples, got {indexed.GetNumberOfTuples()}")
    check(indexed.GetNumberOfComponents() == 1,
          f"Expected 1 component, got {indexed.GetNumberOfComponents()}")

    result = numpy.array(indexed)
    expected = numpy.array([300.0, 100.0])
    assert_array_equal(result, expected)


def test_iter():
    """Test iteration."""
    indexed = make_indexed([10.0, 20.0, 30.0], [2, 0])

    vals = list(indexed)
    check(len(vals) == 2, f"Expected 2 values, got {len(vals)}")
    check(vals[0] == 30.0, f"Expected first value 30.0, got {vals[0]}")
    check(vals[1] == 10.0, f"Expected second value 10.0, got {vals[1]}")


def test_duplicate_indexes():
    """Test that duplicate indexes work correctly."""
    indexed = make_indexed([10.0, 20.0, 30.0], [1, 1, 1, 0, 0])

    result = numpy.array(indexed)
    expected = numpy.array([20.0, 20.0, 20.0, 10.0, 10.0])
    assert_array_equal(result, expected)


def test_lazy_unary_ufunc():
    """Test that unary ufuncs stay lazy (return VTKIndexedArray)."""
    indexed = make_indexed([1.0, 4.0, 9.0, 16.0, 25.0], [2, 4, 0])

    result = numpy.sqrt(indexed)
    check(isinstance(result, VTKIndexedArray),
          f"Expected VTKIndexedArray from sqrt, got {type(result)}")

    # Values should be correct
    expected = numpy.array([3.0, 5.0, 1.0])
    assert_array_almost_equal(numpy.asarray(result), expected)

    # Negative should also stay lazy
    result = -indexed
    check(isinstance(result, VTKIndexedArray),
          f"Expected VTKIndexedArray from negation, got {type(result)}")
    expected = numpy.array([-9.0, -25.0, -1.0])
    assert_array_equal(numpy.asarray(result), expected)


def test_lazy_scalar_binary_ufunc():
    """Test that binary ufuncs with scalar stay lazy."""
    indexed = make_indexed([10.0, 20.0, 30.0, 40.0, 50.0], [4, 2, 0])

    result = indexed + 100
    check(isinstance(result, VTKIndexedArray),
          f"Expected VTKIndexedArray from +scalar, got {type(result)}")
    expected = numpy.array([150.0, 130.0, 110.0])
    assert_array_equal(numpy.asarray(result), expected)

    result = indexed * 2
    check(isinstance(result, VTKIndexedArray),
          f"Expected VTKIndexedArray from *scalar, got {type(result)}")
    expected = numpy.array([100.0, 60.0, 20.0])
    assert_array_equal(numpy.asarray(result), expected)

    # Reverse order should also work
    result = 5 + indexed
    check(isinstance(result, VTKIndexedArray),
          f"Expected VTKIndexedArray from scalar+, got {type(result)}")
    expected = numpy.array([55.0, 35.0, 15.0])
    assert_array_equal(numpy.asarray(result), expected)


def test_materialize_array_binary_ufunc():
    """Test that binary ufuncs with array operand materialize."""
    indexed = make_indexed([10.0, 20.0, 30.0, 40.0, 50.0], [4, 2, 0])
    np_arr = numpy.array([1.0, 2.0, 3.0])

    result = indexed + np_arr
    # Should NOT be VTKIndexedArray (can't stay lazy with array operand)
    check(not isinstance(result, VTKIndexedArray),
          f"Expected non-VTKIndexedArray from +array, got {type(result)}")
    expected = numpy.array([51.0, 32.0, 13.0])
    assert_array_equal(numpy.asarray(result), expected)


def test_lazy_slice_indexing():
    """Test that slice indexing stays lazy (returns VTKIndexedArray)."""
    # base = [10, 20, 30, 40, 50], indexes = [4,3,2,1,0] -> [50,40,30,20,10]
    indexed = make_indexed([10.0, 20.0, 30.0, 40.0, 50.0], [4, 3, 2, 1, 0])

    result = indexed[1:4]
    check(isinstance(result, VTKIndexedArray),
          f"Expected VTKIndexedArray from slice, got {type(result)}")
    expected = numpy.array([40.0, 30.0, 20.0])
    assert_array_equal(numpy.asarray(result), expected)

    # Slice with step
    result = indexed[::2]
    check(isinstance(result, VTKIndexedArray),
          f"Expected VTKIndexedArray from step slice, got {type(result)}")
    expected = numpy.array([50.0, 30.0, 10.0])
    assert_array_equal(numpy.asarray(result), expected)

    # Empty slice
    result = indexed[3:3]
    check(isinstance(result, VTKIndexedArray),
          f"Expected VTKIndexedArray from empty slice, got {type(result)}")
    check(len(result) == 0,
          f"Expected 0 tuples from empty slice, got {len(result)}")


def test_lazy_fancy_indexing():
    """Test that fancy int indexing stays lazy."""
    indexed = make_indexed([10.0, 20.0, 30.0, 40.0, 50.0], [4, 3, 2, 1, 0])

    result = indexed[[0, 2, 4]]
    check(isinstance(result, VTKIndexedArray),
          f"Expected VTKIndexedArray from fancy index, got {type(result)}")
    expected = numpy.array([50.0, 30.0, 10.0])
    assert_array_equal(numpy.asarray(result), expected)

    # Negative fancy indexes
    result = indexed[[-1, -3]]
    check(isinstance(result, VTKIndexedArray),
          f"Expected VTKIndexedArray from negative fancy index, got {type(result)}")
    expected = numpy.array([10.0, 30.0])
    assert_array_equal(numpy.asarray(result), expected)


def test_lazy_boolean_indexing():
    """Test that boolean mask indexing stays lazy."""
    indexed = make_indexed([10.0, 20.0, 30.0, 40.0, 50.0], [4, 3, 2, 1, 0])

    mask = numpy.array([True, False, True, False, True])
    result = indexed[mask]
    check(isinstance(result, VTKIndexedArray),
          f"Expected VTKIndexedArray from boolean mask, got {type(result)}")
    expected = numpy.array([50.0, 30.0, 10.0])
    assert_array_equal(numpy.asarray(result), expected)


def test_lazy_slice_multicomponent():
    """Test that slice indexing stays lazy for multi-component arrays."""
    # base: 4 tuples, 2 comps = [1,2, 3,4, 5,6, 7,8]
    # indexes: [6,7, 2,3, 0,1] -> tuples (7,8), (3,4), (1,2)
    indexed = make_indexed([1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0],
                           [6, 7, 2, 3, 0, 1], ncomps=2)

    result = indexed[0:2]
    check(isinstance(result, VTKIndexedArray),
          f"Expected VTKIndexedArray from mc slice, got {type(result)}")
    expected = numpy.array([[7.0, 8.0], [3.0, 4.0]])
    assert_array_equal(numpy.asarray(result), expected)


def test_chained_lazy_ops():
    """Test that chained lazy operations compose correctly."""
    indexed = make_indexed([10.0, 20.0, 30.0, 40.0, 50.0], [4, 3, 2, 1, 0])

    # sqrt then slice — both stay lazy
    result = numpy.sqrt(indexed)
    check(isinstance(result, VTKIndexedArray),
          f"Expected VTKIndexedArray from sqrt, got {type(result)}")
    result2 = result[1:3]
    check(isinstance(result2, VTKIndexedArray),
          f"Expected VTKIndexedArray from slice of sqrt, got {type(result2)}")
    expected = numpy.sqrt(numpy.array([40.0, 30.0]))
    assert_array_almost_equal(numpy.asarray(result2), expected)


# ---- Run all tests ----

test_override_detection()
test_accessor_properties()
test_properties()
test_materialization()
test_to_numpy()
test_scalar_arithmetic()
test_indexed_numpy_arithmetic()
test_reductions()
test_reduction_methods()
test_ufuncs()
test_scalar_indexing()
test_slice_indexing()
test_multicomponent()
test_readonly()
test_repr()
test_comparison_operators()
test_construction_from_python()
test_iter()
test_duplicate_indexes()
test_lazy_unary_ufunc()
test_lazy_scalar_binary_ufunc()
test_materialize_array_binary_ufunc()
test_lazy_slice_indexing()
test_lazy_fancy_indexing()
test_lazy_boolean_indexing()
test_lazy_slice_multicomponent()
test_chained_lazy_ops()

if errors:
    print(f"\n{errors} error(s) found!")
    sys.exit(1)
else:
    print("All tests passed.")
