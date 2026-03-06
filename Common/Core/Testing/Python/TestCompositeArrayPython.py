# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
"""Tests for VTKCompositeArray numpy-compatible mixin."""

import sys

import numpy
from numpy.testing import assert_array_almost_equal, assert_array_equal

from vtkmodules.vtkCommonCore import (
    vtkAOSDataArrayTemplate,
    vtkCompositeArray,
    vtkConstantArray,
    vtkAffineArray,
    vtkDataArrayCollection,
)
from vtkmodules.numpy_interface.vtk_composite_array import VTKCompositeArray

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


def make_composite(sub_arrays):
    """Create a vtkCompositeArray from a list of sub-arrays."""
    if not sub_arrays:
        return vtkCompositeArray['float64']()

    ncomps = sub_arrays[0].GetNumberOfComponents()
    collection = vtkDataArrayCollection()
    total_tuples = 0
    for arr in sub_arrays:
        collection.AddItem(arr)
        total_tuples += arr.GetNumberOfTuples()

    comp = vtkCompositeArray['float64']()
    comp.SetNumberOfComponents(ncomps)
    comp.SetNumberOfTuples(total_tuples)
    comp.ConstructBackend(collection)
    return comp


def test_override_detection():
    """Test that composite arrays get the VTKCompositeArray mixin."""
    arr1 = make_aos_array([1.0, 2.0, 3.0])
    arr2 = make_aos_array([4.0, 5.0])
    comp = make_composite([arr1, arr2])

    check(isinstance(comp, VTKCompositeArray),
          f"Expected VTKCompositeArray, got {type(comp)}")


def test_sub_array_access():
    """Test the arrays property returns original sub-arrays."""
    arr1 = make_aos_array([1.0, 2.0, 3.0])
    arr2 = make_aos_array([4.0, 5.0])
    comp = make_composite([arr1, arr2])

    sub_arrays = comp.arrays
    check(len(sub_arrays) == 2, f"Expected 2 sub-arrays, got {len(sub_arrays)}")
    check(sub_arrays[0].GetNumberOfTuples() == 3,
          f"First sub-array should have 3 tuples, got {sub_arrays[0].GetNumberOfTuples()}")
    check(sub_arrays[1].GetNumberOfTuples() == 2,
          f"Second sub-array should have 2 tuples, got {sub_arrays[1].GetNumberOfTuples()}")


def test_properties():
    """Test shape, dtype, ndim, size, len."""
    arr1 = make_aos_array([1.0, 2.0, 3.0])
    arr2 = make_aos_array([4.0, 5.0])
    comp = make_composite([arr1, arr2])

    check(comp.shape == (5,), f"Expected shape (5,), got {comp.shape}")
    check(comp.dtype == numpy.float64, f"Expected float64, got {comp.dtype}")
    check(comp.ndim == 1, f"Expected ndim 1, got {comp.ndim}")
    check(comp.size == 5, f"Expected size 5, got {comp.size}")
    check(len(comp) == 5, f"Expected len 5, got {len(comp)}")


def test_properties_multicomponent():
    """Test shape for multi-component arrays."""
    arr1 = make_aos_array([1.0, 2.0, 3.0, 4.0, 5.0, 6.0], ncomps=3)
    arr2 = make_aos_array([7.0, 8.0, 9.0], ncomps=3)
    comp = make_composite([arr1, arr2])

    check(comp.shape == (3, 3), f"Expected shape (3, 3), got {comp.shape}")
    check(comp.ndim == 2, f"Expected ndim 2, got {comp.ndim}")
    check(comp.size == 9, f"Expected size 9, got {comp.size}")
    check(len(comp) == 3, f"Expected len 3, got {len(comp)}")


def test_materialization():
    """Test np.array(comp) matches manual concatenation."""
    arr1 = make_aos_array([1.0, 2.0, 3.0])
    arr2 = make_aos_array([4.0, 5.0])
    comp = make_composite([arr1, arr2])

    result = numpy.array(comp)
    expected = numpy.array([1.0, 2.0, 3.0, 4.0, 5.0])
    assert_array_equal(result, expected)


def test_materialization_multicomponent():
    """Test materialization for multi-component arrays."""
    arr1 = make_aos_array([1.0, 2.0, 3.0, 4.0], ncomps=2)
    arr2 = make_aos_array([5.0, 6.0], ncomps=2)
    comp = make_composite([arr1, arr2])

    result = numpy.array(comp)
    expected = numpy.array([[1.0, 2.0], [3.0, 4.0], [5.0, 6.0]])
    assert_array_equal(result, expected)


def test_to_numpy():
    """Test to_numpy() method."""
    arr1 = make_aos_array([1.0, 2.0, 3.0])
    arr2 = make_aos_array([4.0, 5.0])
    comp = make_composite([arr1, arr2])

    result = comp.to_numpy()
    expected = numpy.array([1.0, 2.0, 3.0, 4.0, 5.0])
    assert_array_equal(result, expected)


def test_scalar_arithmetic():
    """Test arithmetic with scalars."""
    arr1 = make_aos_array([1.0, 2.0, 3.0])
    arr2 = make_aos_array([4.0, 5.0])
    comp = make_composite([arr1, arr2])

    result = numpy.asarray(comp + 10)
    expected = numpy.array([11.0, 12.0, 13.0, 14.0, 15.0])
    assert_array_equal(result, expected)

    result = numpy.asarray(comp * 2)
    expected = numpy.array([2.0, 4.0, 6.0, 8.0, 10.0])
    assert_array_equal(result, expected)


def test_composite_composite_arithmetic():
    """Test arithmetic between two composite arrays."""
    arr1 = make_aos_array([1.0, 2.0, 3.0])
    arr2 = make_aos_array([4.0, 5.0])
    comp1 = make_composite([arr1, arr2])

    arr3 = make_aos_array([10.0, 20.0, 30.0])
    arr4 = make_aos_array([40.0, 50.0])
    comp2 = make_composite([arr3, arr4])

    result = numpy.asarray(comp1 + comp2)
    expected = numpy.array([11.0, 22.0, 33.0, 44.0, 55.0])
    assert_array_equal(result, expected)


def test_composite_numpy_arithmetic():
    """Test arithmetic between composite and numpy array."""
    arr1 = make_aos_array([1.0, 2.0, 3.0])
    arr2 = make_aos_array([4.0, 5.0])
    comp = make_composite([arr1, arr2])

    np_arr = numpy.array([10.0, 20.0, 30.0, 40.0, 50.0])
    result = numpy.asarray(comp + np_arr)
    expected = numpy.array([11.0, 22.0, 33.0, 44.0, 55.0])
    assert_array_equal(result, expected)


def test_reductions():
    """Test sum, min, max, mean reductions."""
    arr1 = make_aos_array([1.0, 2.0, 3.0])
    arr2 = make_aos_array([4.0, 5.0])
    comp = make_composite([arr1, arr2])

    check(numpy.sum(comp) == 15.0,
          f"Expected sum 15.0, got {numpy.sum(comp)}")
    check(numpy.min(comp) == 1.0,
          f"Expected min 1.0, got {numpy.min(comp)}")
    check(numpy.max(comp) == 5.0,
          f"Expected max 5.0, got {numpy.max(comp)}")
    check(numpy.mean(comp) == 3.0,
          f"Expected mean 3.0, got {numpy.mean(comp)}")


def test_reduction_methods():
    """Test .sum(), .min(), .max(), .mean() methods."""
    arr1 = make_aos_array([1.0, 2.0, 3.0])
    arr2 = make_aos_array([4.0, 5.0])
    comp = make_composite([arr1, arr2])

    check(comp.sum() == 15.0, f"Expected sum 15.0, got {comp.sum()}")
    check(comp.min() == 1.0, f"Expected min 1.0, got {comp.min()}")
    check(comp.max() == 5.0, f"Expected max 5.0, got {comp.max()}")
    check(comp.mean() == 3.0, f"Expected mean 3.0, got {comp.mean()}")


def test_std_var():
    """Test std and var reductions."""
    arr1 = make_aos_array([1.0, 2.0, 3.0])
    arr2 = make_aos_array([4.0, 5.0])
    comp = make_composite([arr1, arr2])

    materialized = numpy.array([1.0, 2.0, 3.0, 4.0, 5.0])
    check(abs(comp.std() - materialized.std()) < 1e-10,
          f"std mismatch: {comp.std()} vs {materialized.std()}")
    check(abs(comp.var() - materialized.var()) < 1e-10,
          f"var mismatch: {comp.var()} vs {materialized.var()}")


def test_any_all():
    """Test any and all reductions."""
    arr1 = make_aos_array([0.0, 0.0])
    arr2 = make_aos_array([1.0, 0.0])
    comp = make_composite([arr1, arr2])

    check(comp.any() == True, "Expected any() True")
    check(comp.all() == False, "Expected all() False")

    arr3 = make_aos_array([1.0, 2.0])
    arr4 = make_aos_array([3.0])
    comp2 = make_composite([arr3, arr4])
    check(comp2.all() == True, "Expected all() True for nonzero")


def test_ufuncs():
    """Test numpy ufuncs (sin, sqrt, add)."""
    arr1 = make_aos_array([0.0, 1.0])
    arr2 = make_aos_array([4.0, 9.0])
    comp = make_composite([arr1, arr2])

    result = numpy.asarray(numpy.sqrt(comp))
    expected = numpy.sqrt(numpy.array([0.0, 1.0, 4.0, 9.0]))
    assert_array_almost_equal(result, expected)

    result = numpy.asarray(numpy.sin(comp))
    expected = numpy.sin(numpy.array([0.0, 1.0, 4.0, 9.0]))
    assert_array_almost_equal(result, expected)


def test_scalar_indexing():
    """Test element indexing."""
    arr1 = make_aos_array([10.0, 20.0, 30.0])
    arr2 = make_aos_array([40.0, 50.0])
    comp = make_composite([arr1, arr2])

    check(comp[0] == 10.0, f"Expected comp[0]=10.0, got {comp[0]}")
    check(comp[2] == 30.0, f"Expected comp[2]=30.0, got {comp[2]}")
    check(comp[3] == 40.0, f"Expected comp[3]=40.0, got {comp[3]}")
    check(comp[4] == 50.0, f"Expected comp[4]=50.0, got {comp[4]}")
    check(comp[-1] == 50.0, f"Expected comp[-1]=50.0, got {comp[-1]}")


def test_slice_indexing():
    """Test slice indexing."""
    arr1 = make_aos_array([10.0, 20.0, 30.0])
    arr2 = make_aos_array([40.0, 50.0])
    comp = make_composite([arr1, arr2])

    result = comp[1:4]
    expected = numpy.array([20.0, 30.0, 40.0])
    assert_array_equal(result, expected)

    result = comp[:3]
    expected = numpy.array([10.0, 20.0, 30.0])
    assert_array_equal(result, expected)

    result = comp[3:]
    expected = numpy.array([40.0, 50.0])
    assert_array_equal(result, expected)


def test_multicomponent_indexing():
    """Test component access for multi-component arrays."""
    arr1 = make_aos_array([1.0, 2.0, 3.0, 4.0], ncomps=2)
    arr2 = make_aos_array([5.0, 6.0], ncomps=2)
    comp = make_composite([arr1, arr2])

    row = comp[0]
    assert_array_equal(row, numpy.array([1.0, 2.0]))

    row = comp[2]
    assert_array_equal(row, numpy.array([5.0, 6.0]))


def test_readonly():
    """Test that __setitem__ raises TypeError."""
    arr1 = make_aos_array([1.0, 2.0])
    comp = make_composite([arr1])

    try:
        comp[0] = 99.0
        check(False, "__setitem__ should raise TypeError")
    except TypeError:
        pass


def test_repr():
    """Test __repr__ output."""
    arr1 = make_aos_array([1.0, 2.0])
    arr2 = make_aos_array([3.0])
    comp = make_composite([arr1, arr2])

    r = repr(comp)
    check("VTKCompositeArray" in r, f"Expected VTKCompositeArray in repr, got {r}")
    check("n_arrays=2" in r, f"Expected n_arrays=2 in repr, got {r}")


def test_mixed_sub_array_types():
    """Test composite with mixed sub-array types (constant + AOS)."""
    # Constant array with 3 tuples, value 5.0
    const = vtkConstantArray['float64']()
    const.SetNumberOfComponents(1)
    const.SetNumberOfTuples(3)
    const.ConstructBackend(5.0)

    # AOS array with 2 tuples
    aos = make_aos_array([10.0, 20.0])

    comp = make_composite([const, aos])

    result = numpy.array(comp)
    expected = numpy.array([5.0, 5.0, 5.0, 10.0, 20.0])
    assert_array_equal(result, expected)

    check(numpy.sum(comp) == 45.0,
          f"Expected sum 45.0, got {numpy.sum(comp)}")


def test_mixed_affine_aos():
    """Test composite with affine + AOS sub-arrays."""
    # Affine array: slope=2, intercept=1 -> 1, 3, 5, 7
    affine = vtkAffineArray['float64']()
    affine.SetNumberOfComponents(1)
    affine.SetNumberOfTuples(4)
    affine.ConstructBackend(2.0, 1.0)

    # AOS array
    aos = make_aos_array([100.0, 200.0])

    comp = make_composite([affine, aos])

    result = numpy.array(comp)
    expected = numpy.array([1.0, 3.0, 5.0, 7.0, 100.0, 200.0])
    assert_array_equal(result, expected)

    check(numpy.min(comp) == 1.0, f"Expected min 1.0, got {numpy.min(comp)}")
    check(numpy.max(comp) == 200.0, f"Expected max 200.0, got {numpy.max(comp)}")


def test_offsets():
    """Test that GetOffset returns correct tuple offsets."""
    arr1 = make_aos_array([1.0, 2.0, 3.0])
    arr2 = make_aos_array([4.0, 5.0])
    arr3 = make_aos_array([6.0])
    comp = make_composite([arr1, arr2, arr3])

    check(comp.GetOffset(0) == 0, f"Expected offset 0, got {comp.GetOffset(0)}")
    check(comp.GetOffset(1) == 3, f"Expected offset 3, got {comp.GetOffset(1)}")
    check(comp.GetOffset(2) == 5, f"Expected offset 5, got {comp.GetOffset(2)}")


def test_iter():
    """Test iteration over tuples."""
    arr1 = make_aos_array([1.0, 2.0])
    arr2 = make_aos_array([3.0])
    comp = make_composite([arr1, arr2])

    vals = list(comp)
    check(len(vals) == 3, f"Expected 3 values, got {len(vals)}")
    check(vals[0] == 1.0, f"Expected first value 1.0, got {vals[0]}")
    check(vals[2] == 3.0, f"Expected last value 3.0, got {vals[2]}")


def test_comparison_operators():
    """Test comparison operators."""
    arr1 = make_aos_array([1.0, 2.0, 3.0])
    arr2 = make_aos_array([4.0, 5.0])
    comp = make_composite([arr1, arr2])

    result = numpy.asarray(comp > 3)
    expected = numpy.array([False, False, False, True, True])
    assert_array_equal(result, expected)


def test_constructor_from_iterable():
    """Test construction from an iterable of arrays."""
    arr1 = make_aos_array([1.0, 2.0, 3.0])
    arr2 = make_aos_array([4.0, 5.0])

    comp = vtkCompositeArray['float64']([arr1, arr2])

    check(isinstance(comp, VTKCompositeArray),
          f"Expected VTKCompositeArray, got {type(comp)}")
    check(comp.GetNumberOfTuples() == 5,
          f"Expected 5 tuples, got {comp.GetNumberOfTuples()}")
    check(comp.GetNumberOfComponents() == 1,
          f"Expected 1 component, got {comp.GetNumberOfComponents()}")
    check(comp.GetNumberOfArrays() == 2,
          f"Expected 2 sub-arrays, got {comp.GetNumberOfArrays()}")

    result = numpy.array(comp)
    expected = numpy.array([1.0, 2.0, 3.0, 4.0, 5.0])
    assert_array_equal(result, expected)


def test_constructor_multicomponent():
    """Test construction from multi-component arrays."""
    arr1 = make_aos_array([1.0, 2.0, 3.0, 4.0], ncomps=2)
    arr2 = make_aos_array([5.0, 6.0], ncomps=2)

    comp = vtkCompositeArray['float64']([arr1, arr2])

    check(comp.shape == (3, 2), f"Expected shape (3, 2), got {comp.shape}")
    result = numpy.array(comp)
    expected = numpy.array([[1.0, 2.0], [3.0, 4.0], [5.0, 6.0]])
    assert_array_equal(result, expected)


def test_constructor_generator():
    """Test construction from a generator (non-list iterable)."""
    arrays = [make_aos_array([float(i)]) for i in range(3)]

    comp = vtkCompositeArray['float64'](a for a in arrays)

    check(comp.GetNumberOfTuples() == 3,
          f"Expected 3 tuples, got {comp.GetNumberOfTuples()}")
    result = numpy.array(comp)
    expected = numpy.array([0.0, 1.0, 2.0])
    assert_array_equal(result, expected)


def test_constructor_mixed_types():
    """Test construction from mixed sub-array types."""
    const = vtkConstantArray['float64']()
    const.SetNumberOfComponents(1)
    const.SetNumberOfTuples(2)
    const.ConstructBackend(7.0)

    aos = make_aos_array([10.0, 20.0])

    comp = vtkCompositeArray['float64']([const, aos])

    result = numpy.array(comp)
    expected = numpy.array([7.0, 7.0, 10.0, 20.0])
    assert_array_equal(result, expected)


# ---- Run all tests ----

test_override_detection()
test_sub_array_access()
test_properties()
test_properties_multicomponent()
test_materialization()
test_materialization_multicomponent()
test_to_numpy()
test_scalar_arithmetic()
test_composite_composite_arithmetic()
test_composite_numpy_arithmetic()
test_reductions()
test_reduction_methods()
test_std_var()
test_any_all()
test_ufuncs()
test_scalar_indexing()
test_slice_indexing()
test_multicomponent_indexing()
test_readonly()
test_repr()
test_mixed_sub_array_types()
test_mixed_affine_aos()
test_offsets()
test_iter()
test_comparison_operators()
test_constructor_from_iterable()
test_constructor_multicomponent()
test_constructor_generator()
test_constructor_mixed_types()

if errors:
    print(f"\n{errors} error(s) found!")
    sys.exit(1)
else:
    print("All tests passed.")
