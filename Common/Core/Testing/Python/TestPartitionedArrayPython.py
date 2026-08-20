# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
"""Tests for the numpy-compatible surface of VTKPartitionedArray.

Covers the pieces VTKPartitionedArray shares with the VTKDataArrayMixin
based array wrappers: to_numpy(), the unary operators, truthiness, repr,
and the __array_function__ fallback for numpy functions that have no
partition-aware override.

Also checks that the partition-aware operations really are computed block
by block, by counting how often the whole array gets materialized.
"""

import gc
import sys
import warnings
import weakref

import numpy
from numpy.testing import assert_array_almost_equal, assert_array_equal

from vtkmodules.vtkCommonCore import vtkAOSDataArrayTemplate, vtkWeakReference
from vtkmodules.vtkCommonDataModel import (
    vtkDataObject,
    vtkImageData,
    vtkMultiBlockDataSet,
)
from vtkmodules.numpy_interface.utils import NoneArray
from vtkmodules.numpy_interface.vtk_partitioned_array import VTKPartitionedArray

errors = 0


def check(condition, msg):
    global errors
    if not condition:
        print("ERROR:", msg)
        errors += 1


def make_aos_array(values, ncomps=1):
    """Create an AOS array from a flat list of values."""
    arr = vtkAOSDataArrayTemplate['float64']()
    arr.SetNumberOfComponents(ncomps)
    arr.SetNumberOfTuples(len(values) // ncomps)
    for i, v in enumerate(values):
        arr.SetValue(i, v)
    return arr


# Count whole-array materializations. numpy.asarray() on an individual block
# is a zero-copy view of VTK memory and is deliberately not counted; only
# VTKPartitionedArray.__array__, which concatenates every block, is.
_ARRAY_CALLS = [0]
_ORIGINAL_ARRAY = VTKPartitionedArray.__array__


def _counting_array(self, dtype=None, copy=None):
    _ARRAY_CALLS[0] += 1
    return _ORIGINAL_ARRAY(self, dtype=dtype, copy=copy)


VTKPartitionedArray.__array__ = _counting_array


def call_and_count(func):
    """Return (result, number of whole-array materializations)."""
    _ARRAY_CALLS[0] = 0
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        result = func()
    return result, _ARRAY_CALLS[0]


def make_blocks(blocks, ncomps=1):
    """Build a partitioned array from a list of per-block value lists."""
    return VTKPartitionedArray(
        [make_aos_array(b, ncomps) for b in blocks])


BLOCK0 = [-1.0, 2.0, -3.0]
BLOCK1 = [4.0, -5.0]
FLAT = numpy.array(BLOCK0 + BLOCK1)


def make_partitioned():
    """Two-block partitioned array with no owning dataset."""
    return VTKPartitionedArray(
        [make_aos_array(BLOCK0), make_aos_array(BLOCK1)])


def make_composite_dataset():
    """A vtkMultiBlockDataSet with one scalar array spread over two blocks."""
    mb = vtkMultiBlockDataSet()
    for i, values in enumerate((BLOCK0, BLOCK1)):
        img = vtkImageData()
        img.SetDimensions(len(values), 1, 1)
        arr = make_aos_array(values)
        arr.SetName("scalars")
        img.GetPointData().AddArray(arr)
        mb.SetBlock(i, img)
    return mb


def test_to_numpy():
    """to_numpy() materializes every block into one ndarray."""
    array = make_partitioned()
    result = array.to_numpy()
    check(isinstance(result, numpy.ndarray), "to_numpy should return an ndarray")
    assert_array_almost_equal(result, FLAT)
    check(array.to_numpy(dtype=numpy.float32).dtype == numpy.float32,
          "to_numpy should honor the dtype argument")


def test_unary_operators():
    """Unary operators are applied per block and stay partitioned."""
    array = make_partitioned()
    for label, result, expected in (("neg", -array, -FLAT),
                                    ("pos", +array, FLAT),
                                    ("abs", abs(array), numpy.abs(FLAT))):
        check(isinstance(result, VTKPartitionedArray),
              f"unary {label} should return a VTKPartitionedArray")
        check(len(result.arrays) == 2,
              f"unary {label} should preserve the block count")
        assert_array_almost_equal(numpy.asarray(result), expected)


def test_unary_operators_with_none_array():
    """NoneArray blocks pass through the unary operators untouched."""
    array = VTKPartitionedArray([make_aos_array(BLOCK0), NoneArray])
    result = -array
    check(result.arrays[1] is NoneArray,
          "unary operators should leave NoneArray blocks alone")
    assert_array_almost_equal(numpy.asarray(result), -numpy.array(BLOCK0))


def test_unary_operators_propagate_metadata():
    """Unary operators carry dataset and association to the result."""
    mb = make_composite_dataset()
    array = mb.point_data["scalars"]
    result = -array
    check(result.dataset is array.dataset,
          "unary operators should propagate the dataset")
    check(result.association == vtkDataObject.POINT,
          "unary operators should propagate the association")


def test_bool():
    """Partitioned arrays are always truthy, like the other VTK arrays."""
    check(bool(make_partitioned()) is True,
          "a populated partitioned array should be truthy")
    check(bool(VTKPartitionedArray([])) is True,
          "an empty partitioned array should still be truthy")
    check(bool(VTKPartitionedArray([NoneArray])) is True,
          "an all-NoneArray partitioned array should still be truthy")
    check(bool(NoneArray) is False,
          "NoneArray should remain falsy so 'if array:' keeps working")


def test_repr():
    """repr() reports shape, dtype and block count without materializing."""
    check(repr(make_partitioned()) ==
          "VTKPartitionedArray(shape=(5,), dtype=float64, blocks=2)",
          "repr of a populated partitioned array")
    check(repr(VTKPartitionedArray([])) ==
          "VTKPartitionedArray(shape=(0,), dtype=None, blocks=0)",
          "repr of an empty partitioned array")
    check(repr(VTKPartitionedArray([NoneArray])) ==
          "VTKPartitionedArray(shape=(0,), dtype=None, blocks=1)",
          "repr of an all-NoneArray partitioned array")


def test_shape_of_empty():
    """numpy.shape() of an all-NoneArray array matches an empty one."""
    check(VTKPartitionedArray([]).shape == (0,),
          "shape of an empty partitioned array")
    check(VTKPartitionedArray([NoneArray]).shape == (0,),
          "shape of an all-NoneArray partitioned array")


def test_array_function_fallback():
    """Unregistered numpy functions materialize instead of raising."""
    array = make_partitioned()
    with warnings.catch_warnings(record=True) as caught:
        warnings.simplefilter("always")
        result = numpy.ravel(array)
    assert_array_almost_equal(result, numpy.ravel(FLAT))
    check(len(caught) == 1 and issubclass(caught[0].category, UserWarning),
          "the fallback should warn that all blocks are materialized")

    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        assert_array_almost_equal(numpy.diff(array), numpy.diff(FLAT))
        assert_array_almost_equal(numpy.reshape(array, (-1, 1)),
                                  numpy.reshape(FLAT, (-1, 1)))
        # composite arguments nested in a list
        assert_array_almost_equal(numpy.stack([array, array]),
                                  numpy.stack([FLAT, FLAT]))
        # composite argument in a position other than the first
        assert_array_equal(numpy.searchsorted(numpy.sort(FLAT), array),
                           numpy.searchsorted(numpy.sort(FLAT), FLAT))
        check(abs(numpy.linalg.norm(array) - numpy.linalg.norm(FLAT)) < 1e-9,
              "numpy.linalg.norm through the fallback")


def test_registered_overrides_take_priority():
    """Partition-aware overrides are still preferred over the fallback."""
    array = make_partitioned()
    with warnings.catch_warnings(record=True) as caught:
        warnings.simplefilter("always")
        clipped = numpy.clip(array, -1.0, 1.0)
        total = numpy.sum(array)
    check(not caught, "registered overrides should not warn about materializing")
    check(isinstance(clipped, VTKPartitionedArray),
          "numpy.clip should stay partitioned")
    assert_array_almost_equal(numpy.asarray(clipped), numpy.clip(FLAT, -1.0, 1.0))
    check(abs(total - FLAT.sum()) < 1e-9, "numpy.sum should stay partition-aware")


def test_dataset_is_weakly_referenced():
    """The array must not keep the composite dataset alive."""
    mb = make_composite_dataset()
    array = mb.point_data["scalars"]
    check(array.dataset is mb, "dataset should resolve while the composite is alive")
    check(isinstance(array.__dict__["_dataset"], vtkWeakReference),
          "the dataset back-pointer should be a vtkWeakReference")

    ref = weakref.ref(mb)
    del mb
    gc.collect()
    check(ref() is None, "the composite should be released when the caller drops it")
    check(array.dataset is None, "dataset should be None once the composite is gone")
    # The blocks are resolved during construction, so the data survives.
    assert_array_almost_equal(numpy.asarray(array), FLAT)
    check(abs(array.sum() - FLAT.sum()) < 1e-9,
          "reductions should still work once the composite is gone")


def test_temporary_composite_dataset():
    """Arrays taken from a temporary composite still resolve their blocks."""
    array = make_composite_dataset().point_data["scalars"]
    check(len(array.arrays) == 2, "both blocks should have been resolved")
    assert_array_almost_equal(numpy.asarray(array), FLAT)


def test_array_ufunc_protocol():
    """Unsupported ufunc calls return NotImplemented instead of raising."""
    array = make_partitioned()
    check(VTKPartitionedArray.__array_ufunc__(
              array, numpy.add, "reduce", array) is NotImplemented,
          "methods other than __call__ should return NotImplemented")
    check(VTKPartitionedArray.__array_ufunc__(
              array, numpy.add, "__call__", array, array,
              out=numpy.empty(len(FLAT))) is NotImplemented,
          "an out= argument should return NotImplemented")

    try:
        numpy.add.reduce(array)
        check(False, "numpy.add.reduce should raise")
    except TypeError:
        pass
    except NotImplementedError:
        check(False, "numpy.add.reduce should raise TypeError, not NotImplementedError")

    try:
        numpy.add(array, array, out=numpy.empty(len(FLAT)))
        check(False, "numpy.add with out= should raise")
    except TypeError:
        pass

    # Ordinary ufunc calls are unaffected.
    assert_array_almost_equal(numpy.asarray(numpy.add(array, array)), FLAT + FLAT)


def test_empty_composite():
    """Empty and all-NoneArray composites materialize as empty arrays."""
    for label, array in (("empty", VTKPartitionedArray([])),
                         ("all-NoneArray", VTKPartitionedArray([NoneArray, NoneArray]))):
        materialized = numpy.asarray(array)
        check(isinstance(materialized, numpy.ndarray) and materialized.shape == (0,),
              f"numpy.asarray() of an {label} composite should be an empty array")
        check(array.to_numpy().shape == (0,),
              f"to_numpy() of an {label} composite should be an empty array")
        check(array.shape == (0,), f"shape of an {label} composite")
        check(array.size == 0, f"size of an {label} composite")
    check(numpy.asarray(VTKPartitionedArray([]), dtype=numpy.int32).dtype == numpy.int32,
          "the empty path should honor the requested dtype")

    # A composite where only some blocks hold data still works.
    partial = VTKPartitionedArray([NoneArray, make_aos_array(BLOCK1)])
    assert_array_almost_equal(numpy.asarray(partial), numpy.array(BLOCK1))


def test_copy_matches_numpy_copy():
    """copy() and numpy.copy() agree, and both stay partitioned."""
    array = make_partitioned()
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        method_result = array.copy()
        function_result = numpy.copy(array)
    check(isinstance(method_result, VTKPartitionedArray),
          "copy() should return a VTKPartitionedArray")
    check(isinstance(function_result, VTKPartitionedArray),
          "numpy.copy() should return a VTKPartitionedArray")
    check(len(method_result.arrays) == 2, "copy() should preserve the block count")
    assert_array_almost_equal(numpy.asarray(method_result), FLAT)

    method_result.arrays[0][0] = 99.0
    check(numpy.asarray(array)[0] == BLOCK0[0], "copy() should not alias the original")


def test_size_and_default_arguments():
    """size is a plain int and the default block container is not shared."""
    check(type(make_partitioned().size) is int, "size should be a plain int")
    first, second = VTKPartitionedArray(), VTKPartitionedArray()
    check(first._arrays is not second._arrays,
          "the default arrays argument should not be shared between instances")


# Block layouts that put ties and extremes on block boundaries.
ARG_LAYOUTS = [
    [[3.0, 1.0, 4.0], [1.0, 5.0]],              # min tie spanning two blocks
    [[5.0, 5.0], [1.0, 9.0, 1.0]],              # min tie inside one block
    [[-2.0, 7.0], [0.0], [3.0, -2.0, 8.0]],     # min tie in blocks 1 and 3
    [[1.0]],                                    # single block
    [[2.0, 3.0], [], [4.0]],                    # empty block in the middle
]


def test_argreduce_is_block_wise():
    """argmin/argmax match numpy exactly without materializing."""
    for i, layout in enumerate(ARG_LAYOUTS):
        array = make_blocks(layout)
        reference = numpy.concatenate([numpy.array(b) for b in layout if b])
        for name in ("argmin", "argmax"):
            result, materialized = call_and_count(
                lambda name=name: getattr(numpy, name)(array))
            expected = getattr(numpy, name)(reference)
            check(result == expected,
                  f"layout {i}: numpy.{name}() gave {result}, expected {expected}")
            check(materialized == 0,
                  f"layout {i}: numpy.{name}() should not materialize")


def test_argreduce_axis_zero():
    """argmin/argmax over axis 0 of multi-component blocks."""
    array = make_blocks([[1.0, 9.0, 3.0, 4.0, 5.0, 6.0],
                         [0.0, 8.0, 2.0, 7.0, 1.0, 9.0]], ncomps=3)
    reference = numpy.asarray(array)
    for name in ("argmin", "argmax"):
        result, materialized = call_and_count(
            lambda name=name: getattr(numpy, name)(array, axis=0))
        assert_array_equal(result, getattr(numpy, name)(reference, axis=0))
        check(materialized == 0, f"numpy.{name}(axis=0) should not materialize")


def test_cumulative_is_block_wise():
    """cumsum/cumprod carry across blocks without materializing."""
    array = make_blocks([[1.0, 2.0, 3.0], [4.0, 5.0]])
    reference = numpy.asarray(array)
    for name in ("cumsum", "cumprod"):
        # axis=0 keeps the input shape, so the result stays partitioned.
        result, materialized = call_and_count(
            lambda name=name: getattr(numpy, name)(array, axis=0))
        check(isinstance(result, VTKPartitionedArray),
              f"numpy.{name}(axis=0) should stay partitioned")
        assert_array_almost_equal(numpy.asarray(result),
                                  getattr(numpy, name)(reference, axis=0))
        check(materialized == 0, f"numpy.{name}(axis=0) should not materialize")

        # axis=None flattens, so numpy semantics give one contiguous array.
        result, materialized = call_and_count(
            lambda name=name: getattr(numpy, name)(array))
        assert_array_almost_equal(result, getattr(numpy, name)(reference))
        check(materialized == 0, f"numpy.{name}() should not materialize")


def test_cumulative_with_none_array():
    """Cumulative scans skip NoneArray blocks but keep their slot."""
    array = VTKPartitionedArray([make_aos_array([1.0, 2.0, 3.0, 4.0, 5.0, 6.0], 3),
                                 NoneArray,
                                 make_aos_array([7.0, 8.0, 9.0], 3)])
    reference = numpy.asarray(array)
    result, materialized = call_and_count(lambda: numpy.cumsum(array, axis=0))
    check(result.arrays[1] is NoneArray, "cumsum should keep NoneArray blocks")
    assert_array_almost_equal(numpy.asarray(result),
                              numpy.cumsum(reference, axis=0))
    check(materialized == 0, "cumsum with a NoneArray block should not materialize")


def test_dot_is_block_wise():
    """dot reduces to per-block dot products when the operands line up."""
    a = make_blocks([[1.0, 2.0, 3.0], [4.0, 5.0]])
    b = make_blocks([[2.0, 0.0, 1.0], [1.0, 3.0]])
    expected = numpy.dot(numpy.asarray(a), numpy.asarray(b))

    result, materialized = call_and_count(lambda: numpy.dot(a, b))
    check(abs(result - expected) < 1e-9, "numpy.dot() of two partitioned arrays")
    check(materialized == 0, "numpy.dot() should not materialize aligned operands")

    result, materialized = call_and_count(lambda: a.dot(b))
    check(abs(result - expected) < 1e-9, "the dot() method should agree")
    check(materialized == 0, "the dot() method should not materialize")

    # Tuples times a matrix decomposes by block, so the result stays partitioned.
    vectors = make_blocks([[1.0, 0.0, 0.0, 0.0, 1.0, 0.0], [0.0, 0.0, 1.0]],
                          ncomps=3)
    matrix = numpy.array([[1.0, 2.0], [3.0, 4.0], [5.0, 6.0]])
    reference = numpy.dot(numpy.asarray(vectors), matrix)
    result, materialized = call_and_count(lambda: numpy.dot(vectors, matrix))
    check(isinstance(result, VTKPartitionedArray),
          "numpy.dot(partitioned, matrix) should stay partitioned")
    assert_array_almost_equal(numpy.asarray(result), reference)
    check(materialized == 0, "numpy.dot(partitioned, matrix) should not materialize")

    # Operands whose blocks do not line up still give the right answer.
    misaligned = VTKPartitionedArray([make_aos_array([1.0, 2.0]),
                                      make_aos_array([3.0, 4.0, 5.0])])
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        result = numpy.dot(a, misaligned)
    check(abs(result - numpy.dot(numpy.asarray(a),
                                 numpy.asarray(misaligned))) < 1e-9,
          "misaligned blocks should fall back and stay correct")


def test_norm_is_block_wise():
    """The default 2-norm sums per-block sums of squares."""
    for i, layout in enumerate(ARG_LAYOUTS[:3]):
        array = make_blocks(layout)
        reference = numpy.asarray(array)
        result, materialized = call_and_count(lambda: numpy.linalg.norm(array))
        check(abs(result - numpy.linalg.norm(reference)) < 1e-9,
              f"layout {i}: numpy.linalg.norm() value")
        check(materialized == 0,
              f"layout {i}: numpy.linalg.norm() should not materialize")

    # Other norms materialize but must still be right.
    array = make_blocks([[3.0, -4.0]])
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        check(abs(numpy.linalg.norm(array, ord=1) - 7.0) < 1e-9,
              "ord=1 should fall back correctly")


def test_reductions_are_block_wise():
    """The plain reductions never materialize the whole array."""
    array = make_blocks([[1.0, 2.0, 3.0], [4.0, 5.0]])
    reference = numpy.asarray(array)
    for name in ("sum", "mean", "min", "max", "std", "var", "prod",
                 "any", "all", "count_nonzero", "average"):
        result, materialized = call_and_count(
            lambda name=name: getattr(numpy, name)(array))
        check(materialized == 0, f"numpy.{name}() should not materialize")
        check(numpy.allclose(result, getattr(numpy, name)(reference)),
              f"numpy.{name}() value should match numpy")


test_to_numpy()
test_unary_operators()
test_unary_operators_with_none_array()
test_unary_operators_propagate_metadata()
test_bool()
test_repr()
test_shape_of_empty()
test_array_function_fallback()
test_registered_overrides_take_priority()
test_dataset_is_weakly_referenced()
test_temporary_composite_dataset()
test_array_ufunc_protocol()
test_empty_composite()
test_copy_matches_numpy_copy()
test_size_and_default_arguments()
test_argreduce_is_block_wise()
test_argreduce_axis_zero()
test_cumulative_is_block_wise()
test_cumulative_with_none_array()
test_dot_is_block_wise()
test_norm_is_block_wise()
test_reductions_are_block_wise()

if errors:
    print(f"\n{errors} error(s) found!")
    sys.exit(1)
else:
    print("All tests passed.")
