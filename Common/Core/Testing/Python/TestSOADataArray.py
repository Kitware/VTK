# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
"""Comprehensive tests for VTKSOAArray mixin and numpy_to_vtk_soa."""

from vtkmodules.vtkCommonCore import (
    vtkSOADataArrayTemplate,
    vtkFloatArray,
)
import sys

try:
    import numpy as np
except ImportError:
    import vtkmodules.test.Testing
    print("This test requires numpy!")
    vtkmodules.test.Testing.skip()

from vtkmodules.numpy_interface.vtk_soa_array import VTKSOAArray
from vtkmodules.util.numpy_support import (
    vtk_soa_to_numpy, numpy_to_vtk_soa, get_numpy_array_type,
)

errors = 0


def check(condition, msg):
    global errors
    if not condition:
        print("ERROR:", msg)
        errors += 1


def make_soa(n_tuples=5, n_comps=3, dtype=np.float64):
    """Helper: create a SOA array with known data.

    With the mixin override, the returned array is already a VTKSOAArray.
    """
    _dtype_map = {
        np.float32: 'float32', np.float64: 'float64',
        np.int32: 'int32',
    }
    a = vtkSOADataArrayTemplate[_dtype_map[dtype]]()
    a.SetNumberOfComponents(n_comps)
    a.SetNumberOfTuples(n_tuples)

    # Use VTK's own type mapping to ensure exact dtype match across platforms
    vtk_dtype = np.dtype(get_numpy_array_type(a.GetDataType()))

    comp_arrays = []
    for c in range(n_comps):
        arr = np.arange(c * n_tuples, (c + 1) * n_tuples, dtype=vtk_dtype)
        a.SetArray(c, arr, n_tuples, True, True)
        comp_arrays.append(arr)
    return a, comp_arrays


# -------------------------------------------------------------------
# Test that the override makes SOA arrays into VTKSOAArray instances
# -------------------------------------------------------------------
def test_override():
    vtk_arr, _ = make_soa(5, 3)
    check(isinstance(vtk_arr, VTKSOAArray),
          f"SOA array should be VTKSOAArray instance, got {type(vtk_arr).__name__}")
    # Should also be a vtkDataArray
    from vtkmodules.vtkCommonCore import vtkDataArray
    check(isinstance(vtk_arr, vtkDataArray),
          "SOA array should also be a vtkDataArray instance")


# -------------------------------------------------------------------
# Test properties: shape, ndim, size, dtype, components
# -------------------------------------------------------------------
def test_properties():
    soa, comp_arrays = make_soa(5, 3)

    check(soa.shape == (5, 3), f"shape: expected (5, 3), got {soa.shape}")
    check(soa.ndim == 2, f"ndim: expected 2, got {soa.ndim}")
    check(soa.size == 15, f"size: expected 15, got {soa.size}")
    check(soa.dtype == np.float64, f"dtype: expected float64, got {soa.dtype}")
    check(len(soa.components) == 3, f"components: expected 3, got {len(soa.components)}")
    check(len(soa) == 5, f"len: expected 5, got {len(soa)}")


def test_properties_single_component():
    soa, _ = make_soa(4, 1)

    check(soa.shape == (4,), f"shape: expected (4,), got {soa.shape}")
    check(soa.ndim == 1, f"ndim: expected 1, got {soa.ndim}")
    check(soa.size == 4, f"size: expected 4, got {soa.size}")


# -------------------------------------------------------------------
# Test to_numpy()
# -------------------------------------------------------------------
def test_to_numpy():
    soa, comp_arrays = make_soa(5, 3)
    arr = soa.to_numpy()
    expected = np.column_stack(comp_arrays)
    check(isinstance(arr, np.ndarray), f"to_numpy should return ndarray, got {type(arr).__name__}")
    check(arr.shape == (5, 3), f"to_numpy shape: expected (5, 3), got {arr.shape}")
    check(np.allclose(arr, expected), "to_numpy values mismatch")

    # Single component
    soa1, comp1 = make_soa(4, 1)
    arr1 = soa1.to_numpy()
    check(arr1.shape == (4,), f"to_numpy single-comp shape: expected (4,), got {arr1.shape}")
    check(np.allclose(arr1, comp1[0]), "to_numpy single-comp values mismatch")

    # dtype conversion
    arr32 = soa.to_numpy(dtype=np.float32)
    check(arr32.dtype == np.float32, f"to_numpy dtype: expected float32, got {arr32.dtype}")


# -------------------------------------------------------------------
# Test materialisation: numpy.array(soa) produces correct AOS array
# -------------------------------------------------------------------
def test_materialisation():
    soa, comp_arrays = make_soa(5, 3)
    arr = np.array(soa)

    check(arr.shape == (5, 3), f"materialised shape: expected (5, 3), got {arr.shape}")
    for c in range(3):
        check(np.allclose(arr[:, c], comp_arrays[c]),
              f"materialised component {c} mismatch")


def test_materialisation_single_component():
    soa, comp_arrays = make_soa(4, 1)
    arr = np.array(soa)

    check(arr.shape == (4,), f"materialised shape: expected (4,), got {arr.shape}")
    check(np.allclose(arr, comp_arrays[0]), "materialised single-component mismatch")


# -------------------------------------------------------------------
# Test arithmetic: scalar ops, SOA+SOA, SOA+numpy
# -------------------------------------------------------------------
def test_arithmetic_scalar():
    soa, comp_arrays = make_soa(5, 3)

    result = soa + 10.0
    check(isinstance(result, VTKSOAArray), "scalar add should return VTKSOAArray")
    expected = np.column_stack([c + 10.0 for c in comp_arrays])
    check(np.allclose(np.array(result), expected), "scalar add values mismatch")

    result = soa * 2.0
    check(isinstance(result, VTKSOAArray), "scalar mul should return VTKSOAArray")
    expected = np.column_stack([c * 2.0 for c in comp_arrays])
    check(np.allclose(np.array(result), expected), "scalar mul values mismatch")

    result = 100.0 - soa
    check(isinstance(result, VTKSOAArray), "reverse sub should return VTKSOAArray")
    expected = np.column_stack([100.0 - c for c in comp_arrays])
    check(np.allclose(np.array(result), expected), "reverse sub values mismatch")


def test_arithmetic_soa_soa():
    soa1, comp1 = make_soa(5, 3)
    soa2, comp2 = make_soa(5, 3)

    result = soa1 + soa2
    check(isinstance(result, VTKSOAArray), "SOA+SOA should return VTKSOAArray")
    expected = np.column_stack([a + b for a, b in zip(comp1, comp2)])
    check(np.allclose(np.array(result), expected), "SOA+SOA values mismatch")


def test_arithmetic_soa_numpy():
    soa, comp_arrays = make_soa(5, 3)
    nparr = np.ones((5, 3), dtype=np.float64) * 5.0

    result = soa + nparr
    check(isinstance(result, VTKSOAArray), "SOA+numpy (n,nc) should return VTKSOAArray")
    expected = np.column_stack(comp_arrays) + nparr
    check(np.allclose(np.array(result), expected), "SOA+numpy values mismatch")


# -------------------------------------------------------------------
# Test unary operators
# -------------------------------------------------------------------
def test_unary():
    soa, comp_arrays = make_soa(5, 3)

    neg = -soa
    check(isinstance(neg, VTKSOAArray), "negation should return VTKSOAArray")
    check(np.allclose(np.array(neg), -np.column_stack(comp_arrays)), "negation values mismatch")

    absval = abs(soa)
    check(isinstance(absval, VTKSOAArray), "abs should return VTKSOAArray")
    check(np.allclose(np.array(absval), np.abs(np.column_stack(comp_arrays))), "abs values mismatch")


# -------------------------------------------------------------------
# Test comparison operators
# -------------------------------------------------------------------
def test_comparison():
    soa, comp_arrays = make_soa(5, 3)
    mat = np.column_stack(comp_arrays)

    result = soa > 5.0
    check(isinstance(result, VTKSOAArray), "comparison > scalar should return VTKSOAArray")
    check(result.dtype == np.bool_, f"comparison > dtype: expected bool_, got {result.dtype}")
    check(np.array_equal(np.array(result), mat > 5.0), "comparison > values mismatch")

    result = soa == 0.0
    check(isinstance(result, VTKSOAArray), "comparison == scalar should return VTKSOAArray")
    check(result.dtype == np.bool_, f"comparison == dtype: expected bool_, got {result.dtype}")
    check(np.array_equal(np.array(result), mat == 0.0), "comparison == values mismatch")

    result = soa < 3.0
    check(isinstance(result, VTKSOAArray), "comparison < scalar should return VTKSOAArray")
    check(np.array_equal(np.array(result), mat < 3.0), "comparison < values mismatch")

    result = soa >= 5.0
    check(np.array_equal(np.array(result), mat >= 5.0), "comparison >= values mismatch")

    result = soa <= 5.0
    check(np.array_equal(np.array(result), mat <= 5.0), "comparison <= values mismatch")

    result = soa != 0.0
    check(np.array_equal(np.array(result), mat != 0.0), "comparison != values mismatch")


# -------------------------------------------------------------------
# Test numpy ufuncs
# -------------------------------------------------------------------
def test_ufuncs():
    soa, comp_arrays = make_soa(5, 2)
    # Avoid zero values for log
    soa_shifted = soa + 1.0

    # sin
    result = np.sin(soa)
    check(isinstance(result, VTKSOAArray), "np.sin should return VTKSOAArray")
    check(np.allclose(np.array(result), np.sin(np.column_stack(comp_arrays))),
          "np.sin values mismatch")

    # sqrt
    result = np.sqrt(soa)
    check(isinstance(result, VTKSOAArray), "np.sqrt should return VTKSOAArray")
    check(np.allclose(np.array(result), np.sqrt(np.column_stack(comp_arrays))),
          "np.sqrt values mismatch")

    # exp
    result = np.exp(soa_shifted)
    check(isinstance(result, VTKSOAArray), "np.exp should return VTKSOAArray")

    # Two-arg ufunc: add
    result = np.add(soa, 10.0)
    check(isinstance(result, VTKSOAArray), "np.add should return VTKSOAArray")
    check(np.allclose(np.array(result), np.column_stack(comp_arrays) + 10.0),
          "np.add values mismatch")


# -------------------------------------------------------------------
# Test indexing
# -------------------------------------------------------------------
def test_indexing_element():
    soa, comp_arrays = make_soa(5, 3)

    elem = soa[0]
    expected = np.array([c[0] for c in comp_arrays], dtype=np.float64)
    check(np.allclose(elem, expected), f"element index: {elem} != {expected}")

    elem_neg = soa[-1]
    expected_neg = np.array([c[-1] for c in comp_arrays], dtype=np.float64)
    check(np.allclose(elem_neg, expected_neg), "negative element index mismatch")


def test_indexing_slice():
    soa, comp_arrays = make_soa(5, 3)

    sliced = soa[1:4]
    check(isinstance(sliced, VTKSOAArray), "slice should return VTKSOAArray")
    check(sliced.shape == (3, 3), f"slice shape: expected (3, 3), got {sliced.shape}")
    expected = np.column_stack([c[1:4] for c in comp_arrays])
    check(np.allclose(np.array(sliced), expected), "slice values mismatch")


def test_indexing_component():
    soa, comp_arrays = make_soa(5, 3)

    # Direct component access
    comp0 = soa[:, 0]
    check(np.allclose(comp0, comp_arrays[0]), "component 0 access mismatch")

    comp2 = soa[:, -1]
    check(np.allclose(comp2, comp_arrays[2]), "component -1 access mismatch")

    # Slice + component
    partial = soa[1:3, 1]
    check(np.allclose(partial, comp_arrays[1][1:3]), "slice+component access mismatch")


def test_indexing_fancy():
    soa, comp_arrays = make_soa(5, 3)

    indices = np.array([0, 2, 4])
    result = soa[indices]
    expected = np.column_stack(comp_arrays)[indices]
    check(np.allclose(result, expected), "fancy indexing mismatch")


def test_indexing_boolean():
    soa, comp_arrays = make_soa(5, 3)
    mat = np.column_stack(comp_arrays)

    mask = np.array([True, False, True, False, True])
    result = soa[mask]
    expected = mat[mask]
    check(np.allclose(result, expected), "boolean indexing mismatch")


# -------------------------------------------------------------------
# Test setitem
# -------------------------------------------------------------------
def test_setitem():
    soa, comp_arrays = make_soa(5, 3)

    # Set element
    soa[0] = [99.0, 98.0, 97.0]
    check(soa._component_arrays[0][0] == 99.0, "setitem element comp 0")
    check(soa._component_arrays[1][0] == 98.0, "setitem element comp 1")
    check(soa._component_arrays[2][0] == 97.0, "setitem element comp 2")

    # Set component
    soa[:, 1] = np.zeros(5)
    check(np.allclose(soa._component_arrays[1], 0), "setitem component")

    # Set slice
    soa[1:3] = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float64)
    check(soa._component_arrays[0][1] == 1.0, "setitem slice comp 0")
    check(soa._component_arrays[2][2] == 6.0, "setitem slice comp 2")


# -------------------------------------------------------------------
# Test reduction operations
# -------------------------------------------------------------------
def test_reductions():
    soa, comp_arrays = make_soa(5, 3)
    mat = np.column_stack(comp_arrays)

    # Full sum
    result = np.sum(soa)
    check(np.isclose(result, np.sum(mat)), f"sum: {result} != {np.sum(mat)}")

    # Sum axis=0
    result = np.sum(soa, axis=0)
    expected = np.sum(mat, axis=0)
    check(np.allclose(result, expected), f"sum axis=0: {result} != {expected}")

    # Full min/max
    check(np.isclose(np.min(soa), np.min(mat)), "min mismatch")
    check(np.isclose(np.max(soa), np.max(mat)), "max mismatch")

    # Mean
    result = np.mean(soa, axis=0)
    expected = np.mean(mat, axis=0)
    check(np.allclose(result, expected), f"mean axis=0: {result} != {expected}")


# -------------------------------------------------------------------
# Test zero-copy: modifications through components visible in VTK
# -------------------------------------------------------------------
def test_zero_copy():
    soa, _ = make_soa(5, 2)

    # Modify through component array
    soa.components[0][0] = 999.0
    check(soa.GetTypedComponent(0, 0) == 999.0,
          f"zero-copy: VTK sees {soa.GetTypedComponent(0, 0)}, expected 999.0")

    # Modify through VTK
    soa.SetTypedComponent(1, 1, 777.0)
    check(soa._component_arrays[1][1] == 777.0,
          f"zero-copy reverse: numpy sees {soa._component_arrays[1][1]}, expected 777.0")


# -------------------------------------------------------------------
# Test multiple dtypes
# -------------------------------------------------------------------
def test_dtypes():
    for dtype in [np.float32, np.float64, np.int32]:
        soa, comp_arrays = make_soa(4, 2, dtype=dtype)
        check(soa.dtype == dtype, f"dtype: expected {dtype}, got {soa.dtype}")
        mat = np.array(soa)
        expected = np.column_stack(comp_arrays)
        check(np.allclose(mat, expected), f"dtype {dtype}: materialisation mismatch")


# -------------------------------------------------------------------
# Test astype
# -------------------------------------------------------------------
def test_astype():
    soa, _ = make_soa(5, 2)
    soa32 = soa.astype(np.float32)
    check(soa32.dtype == np.float32, f"astype dtype: expected float32, got {soa32.dtype}")
    check(isinstance(soa32, VTKSOAArray), "astype should return VTKSOAArray")


# -------------------------------------------------------------------
# Test iteration
# -------------------------------------------------------------------
def test_iteration():
    soa, comp_arrays = make_soa(5, 3)
    mat = np.column_stack(comp_arrays)

    for i, row in enumerate(soa):
        check(np.allclose(row, mat[i]), f"iteration row {i} mismatch")


# -------------------------------------------------------------------
# Test auto-detection through data_model
# -------------------------------------------------------------------
def test_auto_detection():
    """SOA arrays should be detected by the data model and returned directly."""
    from vtkmodules.vtkCommonDataModel import vtkPolyData

    soa, _ = make_soa(5, 3)
    soa.SetName("test_soa")

    pd = vtkPolyData()
    pd.GetPointData().AddArray(soa)

    result = pd.point_data["test_soa"]
    check(isinstance(result, VTKSOAArray),
          f"auto-detection: expected VTKSOAArray, got {type(result).__name__}")
    # With mixin, the result should be the same object
    check(result is soa, "auto-detection should return the same object")


def test_auto_detection_aos_fallback():
    """An AOS vtkFloatArray through data_model should not be a VTKSOAArray."""
    from vtkmodules.vtkCommonDataModel import vtkPolyData

    a = vtkFloatArray()
    a.SetNumberOfComponents(2)
    a.SetNumberOfTuples(3)
    a.SetName("test_aos")
    for i in range(3):
        a.SetTuple(i, (float(i), float(i + 10)))

    pd = vtkPolyData()
    pd.GetPointData().AddArray(a)

    result = pd.point_data["test_aos"]
    check(not isinstance(result, VTKSOAArray),
          "AOS array should not be detected as VTKSOAArray")


# -------------------------------------------------------------------
# Test numpy_to_vtk_soa round-trip
# -------------------------------------------------------------------
def test_numpy_to_vtk_soa():
    c0 = np.array([1.0, 2.0, 3.0, 4.0, 5.0], dtype=np.float64)
    c1 = np.array([10.0, 20.0, 30.0, 40.0, 50.0], dtype=np.float64)
    c2 = np.array([100.0, 200.0, 300.0, 400.0, 500.0], dtype=np.float64)

    vtk_arr = numpy_to_vtk_soa([c0, c1, c2], name="test")
    # With override, vtk_arr should be a VTKSOAArray
    check(isinstance(vtk_arr, VTKSOAArray),
          f"numpy_to_vtk_soa should return VTKSOAArray, got {type(vtk_arr).__name__}")
    check(vtk_arr.GetName() == "test", f"name: expected 'test', got '{vtk_arr.GetName()}'")
    check(vtk_arr.GetNumberOfComponents() == 3,
          f"n_comps: expected 3, got {vtk_arr.GetNumberOfComponents()}")
    check(vtk_arr.GetNumberOfTuples() == 5,
          f"n_tuples: expected 5, got {vtk_arr.GetNumberOfTuples()}")

    # Round-trip: vtk -> numpy
    comps = vtk_soa_to_numpy(vtk_arr)
    check(np.allclose(comps[0], c0), "round-trip component 0 mismatch")
    check(np.allclose(comps[1], c1), "round-trip component 1 mismatch")
    check(np.allclose(comps[2], c2), "round-trip component 2 mismatch")


def test_numpy_to_vtk_soa_dtypes():
    for dtype in [np.float32, np.float64, np.int32]:
        c0 = np.array([1, 2, 3], dtype=dtype)
        c1 = np.array([4, 5, 6], dtype=dtype)
        vtk_arr = numpy_to_vtk_soa([c0, c1])
        comps = vtk_soa_to_numpy(vtk_arr)
        check(comps[0].dtype == dtype, f"round-trip dtype {dtype}: got {comps[0].dtype}")
        check(np.allclose(comps[0], c0), f"round-trip dtype {dtype}: comp 0 mismatch")
        check(np.allclose(comps[1], c1), f"round-trip dtype {dtype}: comp 1 mismatch")


def test_numpy_to_vtk_soa_errors():
    # Empty list
    try:
        numpy_to_vtk_soa([])
        check(False, "Expected ValueError for empty list")
    except ValueError:
        pass

    # Mismatched lengths
    try:
        numpy_to_vtk_soa([np.array([1.0, 2.0]), np.array([1.0, 2.0, 3.0])])
        check(False, "Expected ValueError for mismatched lengths")
    except ValueError:
        pass

    # Mismatched dtypes
    try:
        numpy_to_vtk_soa([np.array([1.0], dtype=np.float32),
                           np.array([1.0], dtype=np.float64)])
        check(False, "Expected ValueError for mismatched dtypes")
    except ValueError:
        pass


# -------------------------------------------------------------------
# Test repr/str
# -------------------------------------------------------------------
def test_repr_str():
    soa, _ = make_soa(5, 3)
    r = repr(soa)
    check("VTKSOAArray" in r, f"repr should contain 'VTKSOAArray', got: {r}")
    s = str(soa)
    check(len(s) > 0, "str should not be empty")


# -------------------------------------------------------------------
# Test direct VTK method access (no __getattr__ delegation needed)
# -------------------------------------------------------------------
def test_vtk_direct_access():
    soa, _ = make_soa(5, 3)
    soa.SetName("velocity")
    check(soa.GetName() == "velocity",
          f"GetName: expected 'velocity', got '{soa.GetName()}'")
    check(soa.GetNumberOfTuples() == 5,
          f"GetNumberOfTuples: expected 5, got {soa.GetNumberOfTuples()}")
    # Can pass directly to VTK methods (no .VTKObject needed)
    from vtkmodules.vtkCommonCore import vtkDataArray
    check(isinstance(soa, vtkDataArray),
          "SOA array should be an instance of vtkDataArray")


# -------------------------------------------------------------------
# Run all tests
# -------------------------------------------------------------------
test_override()
test_properties()
test_properties_single_component()
test_to_numpy()
test_materialisation()
test_materialisation_single_component()
test_arithmetic_scalar()
test_arithmetic_soa_soa()
test_arithmetic_soa_numpy()
test_unary()
test_comparison()
test_ufuncs()
test_indexing_element()
test_indexing_slice()
test_indexing_component()
test_indexing_fancy()
test_indexing_boolean()
test_setitem()
test_reductions()
test_zero_copy()
test_dtypes()
test_astype()
test_iteration()
test_auto_detection()
test_auto_detection_aos_fallback()
test_numpy_to_vtk_soa()
test_numpy_to_vtk_soa_dtypes()
test_numpy_to_vtk_soa_errors()
test_repr_str()
test_vtk_direct_access()


# -------------------------------------------------------------------
# Test __array_function__ fallback (np.std, np.concatenate, etc.)
# -------------------------------------------------------------------
def test_array_function_fallback():
    soa, comp_arrays = make_soa(5, 3)
    mat = np.column_stack(comp_arrays)

    # np.std - not in SOA_OVERRIDE, should fall back
    result = np.std(soa)
    check(np.isclose(result, np.std(mat)), f"np.std: {result} != {np.std(mat)}")

    result = np.std(soa, axis=0)
    expected = np.std(mat, axis=0)
    check(np.allclose(result, expected), f"np.std axis=0 mismatch")

    # np.concatenate
    result = np.concatenate([soa, soa])
    expected = np.concatenate([mat, mat])
    check(np.allclose(result, expected), "np.concatenate mismatch")

    # np.clip
    result = np.clip(soa, 2.0, 8.0)
    expected = np.clip(mat, 2.0, 8.0)
    check(np.allclose(result, expected), "np.clip mismatch")

    # np.sort (along axis 0)
    result = np.sort(soa, axis=0)
    expected = np.sort(mat, axis=0)
    check(np.allclose(result, expected), "np.sort mismatch")

    # np.linalg.norm
    result = np.linalg.norm(soa)
    expected = np.linalg.norm(mat)
    check(np.isclose(result, expected), f"np.linalg.norm: {result} != {expected}")

    # np.var
    result = np.var(soa)
    expected = np.var(mat)
    check(np.isclose(result, expected), f"np.var: {result} != {expected}")


# -------------------------------------------------------------------
# Test __array_ufunc__ with mixed SOA + ndarray inputs
# -------------------------------------------------------------------
def test_ufunc_mixed_inputs():
    soa, comp_arrays = make_soa(5, 3)
    mat = np.column_stack(comp_arrays)
    nparr = np.ones((5, 3), dtype=np.float64) * 3.0

    # ufunc: np.add(soa, ndarray)
    result = np.add(soa, nparr)
    expected = np.add(mat, nparr)
    check(np.allclose(result, expected), "np.add(soa, ndarray) mismatch")

    # ufunc: np.multiply(ndarray, soa)
    result = np.multiply(nparr, soa)
    expected = np.multiply(nparr, mat)
    check(np.allclose(result, expected), "np.multiply(ndarray, soa) mismatch")

    # ufunc: np.maximum(soa, ndarray)
    result = np.maximum(soa, nparr)
    expected = np.maximum(mat, nparr)
    check(np.allclose(result, expected), "np.maximum(soa, ndarray) mismatch")


# -------------------------------------------------------------------
# Test comparison between two VTKSOAArrays
# -------------------------------------------------------------------
def test_comparison_soa_soa():
    soa1, comp1 = make_soa(5, 3)
    soa2, comp2 = make_soa(5, 3)
    mat1 = np.column_stack(comp1)
    mat2 = np.column_stack(comp2)

    # Same data => all equal
    result = soa1 >= soa2
    check(isinstance(result, VTKSOAArray), ">= should return VTKSOAArray")
    check(result.dtype == np.bool_, f">= dtype: expected bool_, got {result.dtype}")
    expected = mat1 >= mat2
    check(np.array_equal(np.array(result), expected), ">= between SOA arrays mismatch")
    check(np.array(result).all(), ">= with equal arrays should be all True")

    result = soa1 <= soa2
    check(isinstance(result, VTKSOAArray), "<= should return VTKSOAArray")
    expected = mat1 <= mat2
    check(np.array_equal(np.array(result), expected), "<= between SOA arrays mismatch")

    result = soa1 == soa2
    check(isinstance(result, VTKSOAArray), "== should return VTKSOAArray")
    expected = mat1 == mat2
    check(np.array_equal(np.array(result), expected), "== between SOA arrays mismatch")

    # Different data
    soa3 = soa1 + 1.0
    mat3 = mat1 + 1.0
    result = soa3 > soa1
    check(isinstance(result, VTKSOAArray), "> should return VTKSOAArray")
    expected = mat3 > mat1
    check(np.array_equal(np.array(result), expected), "> between different SOA arrays mismatch")

    result = soa1 < soa3
    check(isinstance(result, VTKSOAArray), "< should return VTKSOAArray")
    expected = mat1 < mat3
    check(np.array_equal(np.array(result), expected), "< between different SOA arrays mismatch")

    result = soa1 != soa3
    check(isinstance(result, VTKSOAArray), "!= should return VTKSOAArray")
    expected = mat1 != mat3
    check(np.array_equal(np.array(result), expected), "!= between different SOA arrays mismatch")


# -------------------------------------------------------------------
# Test ndarray-style methods
# -------------------------------------------------------------------
def test_ndarray_methods():
    soa, comp_arrays = make_soa(5, 3)
    mat = np.column_stack(comp_arrays)

    # Reduction methods
    check(np.isclose(soa.sum(), mat.sum()), f"soa.sum(): {soa.sum()} != {mat.sum()}")
    check(np.isclose(soa.mean(), mat.mean()), f"soa.mean(): {soa.mean()} != {mat.mean()}")
    check(np.isclose(soa.min(), mat.min()), f"soa.min(): {soa.min()} != {mat.min()}")
    check(np.isclose(soa.max(), mat.max()), f"soa.max(): {soa.max()} != {mat.max()}")
    check(np.isclose(soa.std(), mat.std()), f"soa.std(): {soa.std()} != {mat.std()}")
    check(np.isclose(soa.var(), mat.var()), f"soa.var(): {soa.var()} != {mat.var()}")
    check(np.isclose(soa.prod(), mat.prod()), f"soa.prod(): {soa.prod()} != {mat.prod()}")
    check(soa.argmin() == mat.argmin(), f"soa.argmin(): {soa.argmin()} != {mat.argmin()}")
    check(soa.argmax() == mat.argmax(), f"soa.argmax(): {soa.argmax()} != {mat.argmax()}")

    # axis parameter
    check(np.allclose(soa.sum(axis=0), mat.sum(axis=0)), "soa.sum(axis=0) mismatch")
    check(np.allclose(soa.mean(axis=0), mat.mean(axis=0)), "soa.mean(axis=0) mismatch")

    # any / all
    check(soa.any() == mat.any(), "soa.any() mismatch")
    # all() is False since mat contains zeros
    check(soa.all() == mat.all(), "soa.all() mismatch")

    # Shape / layout methods
    check(soa.T.shape == mat.T.shape, f"soa.T shape: {soa.T.shape} != {mat.T.shape}")
    check(np.allclose(soa.T, mat.T), "soa.T values mismatch")

    check(soa.nbytes == sum(c.nbytes for c in comp_arrays), "soa.nbytes mismatch")

    reshaped = soa.reshape(15)
    check(reshaped.shape == (15,), f"reshape: {reshaped.shape}")
    check(np.allclose(reshaped, mat.reshape(15)), "reshape values mismatch")

    flat = soa.flatten()
    check(flat.shape == (15,), f"flatten: {flat.shape}")
    check(np.allclose(flat, mat.flatten()), "flatten values mismatch")

    raveled = soa.ravel()
    check(raveled.shape == (15,), f"ravel: {raveled.shape}")

    copied = soa.copy()
    check(np.allclose(copied, mat), "copy values mismatch")

    lst = soa.tolist()
    check(isinstance(lst, list), "tolist should return list")
    check(len(lst) == 5, f"tolist length: {len(lst)}")

    clipped = soa.clip(2.0, 8.0)
    check(np.allclose(clipped, mat.clip(2.0, 8.0)), "clip values mismatch")

    rounded = soa.round(0)
    check(np.allclose(rounded, mat.round(0)), "round values mismatch")


# -------------------------------------------------------------------
# Test VTK backing always present (IS-A vtkDataArray now)
# -------------------------------------------------------------------
def test_vtk_backing():
    soa, comp_arrays = make_soa(5, 3)

    # Arithmetic result should be a VTKSOAArray (IS-A vtkDataArray)
    result = soa + 1.0
    check(isinstance(result, VTKSOAArray), "arithmetic result should be VTKSOAArray")
    check(result.GetNumberOfTuples() == 5,
          "arithmetic result should have correct tuples")
    check(result.GetNumberOfComponents() == 3,
          "arithmetic result should have correct components")

    # Comparison result should be VTKSOAArray (char SOA for bool)
    result = soa > 5.0
    check(isinstance(result, VTKSOAArray), "comparison result should be VTKSOAArray")
    check(result.dtype == np.bool_, f"comparison dtype should be bool_, got {result.dtype}")
    check(result.GetNumberOfTuples() == 5,
          "comparison should have correct tuples")
    check(result.GetNumberOfComponents() == 3,
          "comparison should have correct components")
    # Verify the bool components are correct
    expected = np.column_stack(comp_arrays) > 5.0
    check(np.array_equal(np.array(result), expected), "comparison with VTK backing values mismatch")

    # Slice result should be VTKSOAArray
    sliced = soa[1:4]
    check(isinstance(sliced, VTKSOAArray), "slice result should be VTKSOAArray")

    # SOA-SOA comparison
    soa2, _ = make_soa(5, 3)
    result = soa >= soa2
    check(isinstance(result, VTKSOAArray), "SOA-SOA comparison should be VTKSOAArray")
    check(result.dtype == np.bool_, "SOA-SOA comparison dtype should be bool_")


# -------------------------------------------------------------------
# Test per-component methods: clip, round, copy
# -------------------------------------------------------------------
def test_per_component_methods():
    soa, comp_arrays = make_soa(5, 3)
    mat = np.column_stack(comp_arrays)

    # clip
    result = soa.clip(2.0, 8.0)
    check(isinstance(result, VTKSOAArray), "clip should return VTKSOAArray")
    check(np.allclose(np.array(result), mat.clip(2.0, 8.0)), "clip values mismatch")

    # round
    soa_frac = soa + 0.7
    mat_frac = mat + 0.7
    result = soa_frac.round(0)
    check(isinstance(result, VTKSOAArray), "round should return VTKSOAArray")
    check(np.allclose(np.array(result), mat_frac.round(0)), "round values mismatch")

    # copy
    result = soa.copy()
    check(isinstance(result, VTKSOAArray), "copy should return VTKSOAArray")
    check(np.allclose(np.array(result), mat), "copy values mismatch")
    # Verify it's actually a copy (modifying original doesn't affect copy)
    result._component_arrays[0][0] = -999.0
    check(soa._component_arrays[0][0] != -999.0, "copy should be independent of original")


# -------------------------------------------------------------------
# Test cumsum and cumprod
# -------------------------------------------------------------------
def test_cumsum_cumprod():
    soa, comp_arrays = make_soa(5, 3)
    mat = np.column_stack(comp_arrays)

    # cumsum axis=0
    result = np.cumsum(soa, axis=0)
    check(isinstance(result, VTKSOAArray), "cumsum axis=0 should return VTKSOAArray")
    expected = np.cumsum(mat, axis=0)
    check(np.allclose(np.array(result), expected), "cumsum axis=0 values mismatch")

    # cumprod axis=0
    soa_ones = soa + 1.0  # avoid zeros
    mat_ones = mat + 1.0
    result = np.cumprod(soa_ones, axis=0)
    check(isinstance(result, VTKSOAArray), "cumprod axis=0 should return VTKSOAArray")
    expected = np.cumprod(mat_ones, axis=0)
    check(np.allclose(np.array(result), expected), "cumprod axis=0 values mismatch")

    # Method style
    result = soa.cumsum(axis=0)
    check(isinstance(result, VTKSOAArray), "soa.cumsum(axis=0) should return VTKSOAArray")
    check(np.allclose(np.array(result), np.cumsum(mat, axis=0)), "soa.cumsum values mismatch")


# -------------------------------------------------------------------
# Test axis=1 reductions
# -------------------------------------------------------------------
def test_axis1_reductions():
    soa, comp_arrays = make_soa(5, 3)
    mat = np.column_stack(comp_arrays)

    # sum axis=1
    result = np.sum(soa, axis=1)
    expected = np.sum(mat, axis=1)
    check(np.allclose(result, expected), f"sum axis=1: {result} != {expected}")

    # mean axis=1
    result = np.mean(soa, axis=1)
    expected = np.mean(mat, axis=1)
    check(np.allclose(result, expected), f"mean axis=1: {result} != {expected}")

    # min axis=1
    result = np.min(soa, axis=1)
    expected = np.min(mat, axis=1)
    check(np.allclose(result, expected), f"min axis=1: {result} != {expected}")

    # max axis=1
    result = np.max(soa, axis=1)
    expected = np.max(mat, axis=1)
    check(np.allclose(result, expected), f"max axis=1: {result} != {expected}")

    # std axis=1
    result = np.std(soa, axis=1)
    expected = np.std(mat, axis=1)
    check(np.allclose(result, expected), f"std axis=1: {result} != {expected}")

    # var axis=1
    result = np.var(soa, axis=1)
    expected = np.var(mat, axis=1)
    check(np.allclose(result, expected), f"var axis=1: {result} != {expected}")

    # prod axis=1
    soa_shifted = soa + 1.0
    mat_shifted = mat + 1.0
    result = np.prod(soa_shifted, axis=1)
    expected = np.prod(mat_shifted, axis=1)
    check(np.allclose(result, expected), f"prod axis=1: {result} != {expected}")

    # any axis=1
    result = np.any(soa, axis=1)
    expected = np.any(mat, axis=1)
    check(np.array_equal(result, expected), "any axis=1 mismatch")

    # all axis=1
    result = np.all(soa, axis=1)
    expected = np.all(mat, axis=1)
    check(np.array_equal(result, expected), "all axis=1 mismatch")

    # argmin axis=1
    result = np.argmin(soa, axis=1)
    expected = np.argmin(mat, axis=1)
    check(np.array_equal(result, expected), f"argmin axis=1: {result} != {expected}")

    # argmax axis=1
    result = np.argmax(soa, axis=1)
    expected = np.argmax(mat, axis=1)
    check(np.array_equal(result, expected), f"argmax axis=1: {result} != {expected}")


# -------------------------------------------------------------------
# Test np.concatenate override
# -------------------------------------------------------------------
def test_concatenate():
    soa1, comp1 = make_soa(5, 3)
    soa2, comp2 = make_soa(4, 3)
    mat1 = np.column_stack(comp1)
    mat2 = np.column_stack(comp2)

    result = np.concatenate([soa1, soa2])
    check(isinstance(result, VTKSOAArray), "concatenate should return VTKSOAArray")
    check(result.shape == (9, 3), f"concatenate shape: expected (9, 3), got {result.shape}")
    expected = np.concatenate([mat1, mat2])
    check(np.allclose(np.array(result), expected), "concatenate values mismatch")


# -------------------------------------------------------------------
# Test np.clip override
# -------------------------------------------------------------------
def test_np_clip():
    soa, comp_arrays = make_soa(5, 3)
    mat = np.column_stack(comp_arrays)

    result = np.clip(soa, 2.0, 8.0)
    check(isinstance(result, VTKSOAArray), "np.clip should return VTKSOAArray")
    check(np.allclose(np.array(result), np.clip(mat, 2.0, 8.0)), "np.clip values mismatch")


# -------------------------------------------------------------------
# Test sort and dot
# -------------------------------------------------------------------
def test_sort():
    soa, comp_arrays = make_soa(5, 3)
    mat = np.column_stack(comp_arrays)

    # sort axis=0 (per-component)
    result = soa.sort(axis=0)
    check(isinstance(result, VTKSOAArray), "sort axis=0 should return VTKSOAArray")
    expected = np.sort(mat, axis=0)
    check(np.allclose(np.array(result), expected), "sort axis=0 values mismatch")

    # np.sort
    result = np.sort(soa, axis=0)
    check(isinstance(result, VTKSOAArray), "np.sort axis=0 should return VTKSOAArray")
    check(np.allclose(np.array(result), expected), "np.sort axis=0 values mismatch")


def test_dot():
    soa, comp_arrays = make_soa(5, 3)
    mat = np.column_stack(comp_arrays)

    # Matrix-vector dot product
    vec = np.array([1.0, 2.0, 3.0])
    result = np.dot(soa, vec)
    expected = np.dot(mat, vec)
    check(np.allclose(result, expected), f"dot product: {result} != {expected}")

    # Method style
    result = soa.dot(vec)
    check(np.allclose(result, expected), "soa.dot values mismatch")


# -------------------------------------------------------------------
# Test ufunc.reduce and ufunc.accumulate
# -------------------------------------------------------------------
def test_ufunc_reduce():
    soa, comp_arrays = make_soa(5, 3)
    mat = np.column_stack(comp_arrays)

    # add.reduce axis=0 (equivalent to sum axis=0)
    result = np.add.reduce(soa, axis=0)
    expected = np.add.reduce(mat, axis=0)
    check(np.allclose(result, expected), f"add.reduce axis=0: {result} != {expected}")

    # add.reduce axis=1 (per-tuple sum across components — the VTK dot pattern)
    result = np.add.reduce(soa, axis=1)
    expected = np.add.reduce(mat, axis=1)
    check(np.allclose(result, expected), f"add.reduce axis=1: {result} != {expected}")

    # add.reduce axis=None (sum of everything)
    result = np.add.reduce(soa, axis=None)
    expected = np.sum(mat)
    check(np.isclose(result, expected), f"add.reduce axis=None: {result} != {expected}")

    # multiply.reduce axis=1 (per-tuple product)
    soa_shifted = soa + 1.0
    mat_shifted = mat + 1.0
    result = np.multiply.reduce(soa_shifted, axis=1)
    expected = np.multiply.reduce(mat_shifted, axis=1)
    check(np.allclose(result, expected), f"multiply.reduce axis=1: {result} != {expected}")

    # maximum.reduce axis=1 (per-tuple max)
    result = np.maximum.reduce(soa, axis=1)
    expected = np.maximum.reduce(mat, axis=1)
    check(np.allclose(result, expected), f"maximum.reduce axis=1: {result} != {expected}")

    # minimum.reduce axis=0
    result = np.minimum.reduce(soa, axis=0)
    expected = np.minimum.reduce(mat, axis=0)
    check(np.allclose(result, expected), f"minimum.reduce axis=0: {result} != {expected}")

    # The VTK per-element dot product pattern: a1*a2 then add.reduce(m, 1)
    soa2, comp2 = make_soa(5, 3)
    mat2 = np.column_stack(comp2)
    m = soa * soa2
    check(isinstance(m, VTKSOAArray), "soa*soa2 should be VTKSOAArray")
    result = np.add.reduce(m, 1)
    expected = np.add.reduce(mat * mat2, 1)
    check(np.allclose(result, expected), f"VTK dot pattern: {result} != {expected}")


def test_ufunc_accumulate():
    soa, comp_arrays = make_soa(5, 3)
    mat = np.column_stack(comp_arrays)

    # add.accumulate axis=0 (equivalent to cumsum)
    result = np.add.accumulate(soa, axis=0)
    check(isinstance(result, VTKSOAArray),
          "add.accumulate axis=0 should return VTKSOAArray")
    expected = np.add.accumulate(mat, axis=0)
    check(np.allclose(np.array(result), expected), "add.accumulate axis=0 values mismatch")

    # multiply.accumulate axis=0 (equivalent to cumprod)
    soa_shifted = soa + 1.0
    mat_shifted = mat + 1.0
    result = np.multiply.accumulate(soa_shifted, axis=0)
    check(isinstance(result, VTKSOAArray),
          "multiply.accumulate axis=0 should return VTKSOAArray")
    expected = np.multiply.accumulate(mat_shifted, axis=0)
    check(np.allclose(np.array(result), expected), "multiply.accumulate axis=0 values mismatch")


# -------------------------------------------------------------------
# Test metadata propagation through element-wise operations
# -------------------------------------------------------------------
def test_metadata_propagation():
    from vtkmodules.vtkCommonDataModel import vtkPolyData, vtkDataObject

    pd = vtkPolyData()
    pd.GetPointData().SetNumberOfTuples(5)

    soa, comp_arrays = make_soa(5, 3)
    soa.SetName("velocity")
    pd.GetPointData().AddArray(soa)

    arr = pd.point_data["velocity"]
    check(isinstance(arr, VTKSOAArray),
          f"Should get VTKSOAArray from data_model, got {type(arr).__name__}")
    check(arr._dataset is not None, "metadata_propagation: _dataset should be set")
    check(arr._association == vtkDataObject.POINT,
          "metadata_propagation: _association should be POINT")

    # Arithmetic: arr + 1
    result = arr + 1
    check(isinstance(result, VTKSOAArray),
          f"arr+1 should be VTKSOAArray, got {type(result).__name__}")
    check(result.dataset is not None, "arr+1 should have DataSet")
    check(result.association == vtkDataObject.POINT, "arr+1 should have Association")

    # Unary: -arr
    result = -arr
    check(isinstance(result, VTKSOAArray),
          f"-arr should be VTKSOAArray, got {type(result).__name__}")
    check(result.dataset is not None, "-arr should have DataSet")
    check(result.association == vtkDataObject.POINT, "-arr should have Association")

    # Unary: abs(arr)
    result = abs(arr)
    check(isinstance(result, VTKSOAArray),
          f"abs(arr) should be VTKSOAArray, got {type(result).__name__}")
    check(result.dataset is not None, "abs(arr) should have DataSet")
    check(result.association == vtkDataObject.POINT, "abs(arr) should have Association")

    # Ufunc: np.sin(arr)
    result = np.sin(arr)
    check(isinstance(result, VTKSOAArray),
          f"np.sin(arr) should be VTKSOAArray, got {type(result).__name__}")
    check(result.dataset is not None, "np.sin(arr) should have DataSet")
    check(result.association == vtkDataObject.POINT, "np.sin(arr) should have Association")

    # Comparison: arr > 0
    result = arr > 0
    check(isinstance(result, VTKSOAArray),
          f"arr>0 should be VTKSOAArray, got {type(result).__name__}")
    check(result.dataset is not None, "arr>0 should have DataSet")
    check(result.association == vtkDataObject.POINT, "arr>0 should have Association")

    # Reductions should NOT have metadata
    result = arr.sum()
    check(not isinstance(result, VTKSOAArray),
          "arr.sum() should be plain scalar, not VTKSOAArray")


# -------------------------------------------------------------------
# Test numpy.where, numpy.isin, numpy.round overrides (per-component)
# -------------------------------------------------------------------
def test_where_3arg():
    soa, comp_arrays = make_soa(5, 3)
    mat = np.column_stack(comp_arrays)

    # 3-arg form: numpy.where(condition, x, y) — per-component
    cond = soa > 5.0
    result = np.where(cond, soa, 0)
    check(isinstance(result, VTKSOAArray),
          "np.where 3-arg should return VTKSOAArray")
    expected = np.where(mat > 5.0, mat, 0)
    check(np.allclose(np.array(result), expected), "np.where 3-arg values mismatch")

    # 1-arg form: numpy.where(condition) — returns index tuples
    result = np.where(soa > 5.0)
    expected = np.where(mat > 5.0)
    check(len(result) == len(expected), "np.where 1-arg tuple length mismatch")
    for r, e in zip(result, expected):
        check(np.array_equal(r, e), "np.where 1-arg index mismatch")


def test_isin():
    soa, comp_arrays = make_soa(5, 3)
    mat = np.column_stack(comp_arrays)

    test_values = [0.0, 1.0, 5.0, 10.0]
    result = np.isin(soa, test_values)
    check(isinstance(result, VTKSOAArray),
          "np.isin should return VTKSOAArray")
    expected = np.isin(mat, test_values)
    check(np.array_equal(np.array(result), expected), "np.isin values mismatch")


def test_round_override():
    soa, comp_arrays = make_soa(5, 3)
    soa_frac = soa + 0.567
    mat_frac = np.column_stack(comp_arrays) + 0.567

    result = np.round(soa_frac, 2)
    check(isinstance(result, VTKSOAArray),
          "np.round should return VTKSOAArray")
    expected = np.round(mat_frac, 2)
    check(np.allclose(np.array(result), expected), "np.round values mismatch")


test_where_3arg()
test_isin()
test_round_override()
test_metadata_propagation()
test_array_function_fallback()
test_ufunc_mixed_inputs()
test_comparison_soa_soa()
test_ndarray_methods()
test_vtk_backing()
test_per_component_methods()
test_cumsum_cumprod()
test_axis1_reductions()
test_concatenate()
test_np_clip()
test_sort()
test_dot()
test_ufunc_reduce()
test_ufunc_accumulate()

if errors:
    print(f"\n{errors} error(s) found.")
    sys.exit(1)
else:
    print("All tests passed.")
