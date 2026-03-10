# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
"""Test VTKConstantArray mixin for vtkConstantArray."""

from vtkmodules.vtkCommonCore import vtkConstantArray
from vtkmodules.numpy_interface.vtk_constant_array import VTKConstantArray
import sys

try:
    import numpy as np
except ImportError:
    import vtkmodules.test.Testing
    print("This test requires numpy!")
    vtkmodules.test.Testing.skip()

errors = 0


def check(condition, msg):
    global errors
    if not condition:
        print("ERROR:", msg)
        errors += 1


def check_close(actual, expected, msg, rtol=1e-7):
    if isinstance(actual, np.ndarray) and isinstance(expected, np.ndarray):
        if actual.shape != expected.shape:
            check(False, f"{msg}: shape mismatch {actual.shape} vs {expected.shape}")
            return
        if not np.allclose(actual, expected, rtol=rtol):
            check(False, f"{msg}: arrays not close\n  actual:   {actual}\n  expected: {expected}")
    elif np.isscalar(actual) and np.isscalar(expected):
        import math
        check(math.isclose(float(actual), float(expected), rel_tol=rtol),
              f"{msg}: {actual} != {expected}")
    else:
        # mixed types: convert both
        if not np.allclose(np.asarray(actual), np.asarray(expected), rtol=rtol):
            check(False, f"{msg}: {actual} != {expected}")


def make_const_array(value, ntuples, ncomps, dtype='float64'):
    """Create a vtkConstantArray (now a VTKConstantArray via mixin)."""
    return vtkConstantArray[dtype]((ntuples, ncomps), value)


# -------------------------------------------------------------------
# Test that the mixin is applied
# -------------------------------------------------------------------
def test_mixin_applied():
    c = make_const_array(5.0, 100, 3)
    check(isinstance(c, VTKConstantArray),
          f"vtkConstantArray should be VTKConstantArray instance, got {type(c)}")
    # IS-A vtkDataArray (not a wrapper)
    from vtkmodules.vtkCommonCore import vtkDataArray
    check(isinstance(c, vtkDataArray),
          "VTKConstantArray should inherit from vtkDataArray")


# -------------------------------------------------------------------
# Test properties
# -------------------------------------------------------------------
def test_properties():
    c = make_const_array(5.0, 100, 3)
    check(c.value == 5.0, "value property")
    check(c.shape == (100, 3), f"shape: expected (100, 3), got {c.shape}")
    check(c.ndim == 2, f"ndim: expected 2, got {c.ndim}")
    check(c.size == 300, f"size: expected 300, got {c.size}")
    check(c.dtype == np.float64, f"dtype: expected float64, got {c.dtype}")

    # 1-component array has 1D shape
    c1 = make_const_array(7.0, 50, 1)
    check(c1.shape == (50,), f"1-comp shape: expected (50,), got {c1.shape}")
    check(c1.ndim == 1, f"1-comp ndim: expected 1, got {c1.ndim}")
    check(len(c1) == 50, f"len: expected 50, got {len(c1)}")


# -------------------------------------------------------------------
# Test __array__ (materialization)
# -------------------------------------------------------------------
def test_array_conversion():
    c = make_const_array(3.0, 10, 3)
    arr = np.asarray(c)
    expected = np.full((10, 3), 3.0)
    check_close(arr, expected, "__array__ multi-component")
    check(arr.dtype == np.float64, f"__array__ dtype: {arr.dtype}")

    c1 = make_const_array(3.0, 10, 1)
    arr1 = np.asarray(c1)
    check(arr1.shape == (10,), f"__array__ 1-comp shape: {arr1.shape}")


# -------------------------------------------------------------------
# Test arithmetic with numpy arrays (ufunc path)
# -------------------------------------------------------------------
def test_arithmetic_with_numpy():
    c = make_const_array(5.0, 4, 1)
    np_arr = np.array([1.0, 2.0, 3.0, 4.0])

    # Forward operators
    check_close(c + np_arr, 5.0 + np_arr, "const + np")
    check_close(c - np_arr, 5.0 - np_arr, "const - np")
    check_close(c * np_arr, 5.0 * np_arr, "const * np")
    check_close(c / (np_arr + 1), 5.0 / (np_arr + 1), "const / np")
    check_close(c ** np_arr, 5.0 ** np_arr, "const ** np")
    check_close(c // np_arr, 5.0 // np_arr, "const // np")
    check_close(c % np_arr, 5.0 % np_arr, "const % np")

    # Reverse operators
    check_close(np_arr + c, np_arr + 5.0, "np + const")
    check_close(np_arr - c, np_arr - 5.0, "np - const")
    check_close(np_arr * c, np_arr * 5.0, "np * const")
    check_close(np_arr / c, np_arr / 5.0, "np / const")
    check_close(np_arr ** c, np_arr ** 5.0, "np ** const")


# -------------------------------------------------------------------
# Test arithmetic with scalars
# -------------------------------------------------------------------
def test_arithmetic_with_scalar():
    c = make_const_array(5.0, 4, 1)

    result = c + 2
    check(isinstance(result, VTKConstantArray),
          "const + scalar should return VTKConstantArray")
    check_close(result.value, 7.0, "const + scalar value")
    check(result.shape == c.shape, f"const + scalar shape: {result.shape}")

    check_close((c * 3).value, 15.0, "const * scalar value")
    check_close((2 + c).value, 7.0, "scalar + const value")
    check_close((3 * c).value, 15.0, "scalar * const value")

    # Multi-component: result must preserve shape
    c3 = make_const_array(5.0, 4, 3)
    result3 = c3 + 2
    check(isinstance(result3, VTKConstantArray),
          "multi-comp const + scalar should return VTKConstantArray")
    check(result3.shape == (4, 3),
          f"multi-comp const + scalar shape: {result3.shape}")
    check_close(result3.value, 7.0, "multi-comp const + scalar value")


# -------------------------------------------------------------------
# Test multi-component arithmetic
# -------------------------------------------------------------------
def test_multi_component_arithmetic():
    c = make_const_array(2.0, 3, 3)
    np_arr = np.arange(9.0).reshape(3, 3)

    result = c + np_arr
    expected = 2.0 + np_arr
    check_close(result, expected, "multi-comp const + np")

    result = np_arr * c
    expected = np_arr * 2.0
    check_close(result, expected, "multi-comp np * const")


# -------------------------------------------------------------------
# Test unary operators
# -------------------------------------------------------------------
def test_unary():
    c = make_const_array(5.0, 4, 1)

    neg = -c
    check(isinstance(neg, VTKConstantArray), "negation returns VTKConstantArray")
    check_close(neg.value, -5.0, "negation value")
    check(neg.shape == c.shape, f"negation shape: {neg.shape}")

    check_close((+c).value, 5.0, "positive value")
    check_close(abs(make_const_array(-3.0, 4, 1)).value, 3.0, "abs value")


# -------------------------------------------------------------------
# Test comparison operators
# -------------------------------------------------------------------
def test_comparison():
    c = make_const_array(5.0, 4, 1)
    np_arr = np.array([3.0, 5.0, 7.0, 5.0])

    check_close((c < np_arr).astype(float), (5.0 < np_arr).astype(float), "const < np")
    check_close((c <= np_arr).astype(float), (5.0 <= np_arr).astype(float), "const <= np")
    check_close((c == np_arr).astype(float), (5.0 == np_arr).astype(float), "const == np")
    check_close((c != np_arr).astype(float), (5.0 != np_arr).astype(float), "const != np")
    check_close((c > np_arr).astype(float), (5.0 > np_arr).astype(float), "const > np")
    check_close((c >= np_arr).astype(float), (5.0 >= np_arr).astype(float), "const >= np")


# -------------------------------------------------------------------
# Test numpy ufuncs directly
# -------------------------------------------------------------------
def test_ufuncs():
    c = make_const_array(0.5, 4, 1)
    np_arr = np.array([0.1, 0.2, 0.3, 0.4])

    # Mixed with real array: returns ndarray or VTKAOSArray
    check_close(np.add(c, np_arr), 0.5 + np_arr, "np.add")
    check_close(np.multiply(c, np_arr), 0.5 * np_arr, "np.multiply")

    # Unary ufuncs: returns VTKConstantArray
    for name, func, expected in [
        ("np.sin",      np.sin,      np.sin(0.5)),
        ("np.cos",      np.cos,      np.cos(0.5)),
        ("np.exp",      np.exp,      np.exp(0.5)),
        ("np.sqrt",     np.sqrt,     np.sqrt(0.5)),
        ("np.log",      np.log,      np.log(0.5)),
        ("np.negative", np.negative, -0.5),
        ("np.square",   np.square,   0.25),
    ]:
        result = func(c)
        check(isinstance(result, VTKConstantArray),
              f"{name} should return VTKConstantArray")
        check_close(result.value, expected, f"{name} value")
        check(result.shape == c.shape, f"{name} shape: {result.shape}")


# -------------------------------------------------------------------
# Test two VTKConstantArrays together
# -------------------------------------------------------------------
def test_const_plus_const():
    c1 = make_const_array(3.0, 4, 1)
    c2 = make_const_array(7.0, 4, 1)

    result = c1 + c2
    check(isinstance(result, VTKConstantArray),
          "const + const should return VTKConstantArray")
    check_close(result.value, 10.0, "const + const value")
    check(result.shape == c1.shape, f"const + const shape: {result.shape}")

    check_close((c1 * c2).value, 21.0, "const * const value")
    check_close((c2 - c1).value, 4.0, "const - const value")

    # Multi-component const + const
    c3 = make_const_array(3.0, 4, 3)
    c4 = make_const_array(7.0, 4, 3)
    result3 = c3 + c4
    check(isinstance(result3, VTKConstantArray),
          "multi-comp const + const should return VTKConstantArray")
    check(result3.shape == (4, 3),
          f"multi-comp const + const shape: {result3.shape}")
    check_close(result3.value, 10.0, "multi-comp const + const value")


# -------------------------------------------------------------------
# Test indexing
# -------------------------------------------------------------------
def test_indexing():
    c = make_const_array(5.0, 10, 3)

    # Single element
    val = c[0, 0]
    check_close(val, 5.0, "c[0, 0]")

    # Row slice — returns constant array
    row = c[0]
    check(isinstance(row, VTKConstantArray),
          "c[0] should return VTKConstantArray")
    check(row.shape == (3,), f"c[0] shape: {row.shape}")
    check_close(row.value, 5.0, "c[0] value")

    # Slice of rows
    sliced = c[2:5]
    check(isinstance(sliced, VTKConstantArray),
          "c[2:5] should return VTKConstantArray")
    check(sliced.shape == (3, 3), f"c[2:5] shape: {sliced.shape}")
    check_close(sliced.value, 5.0, "c[2:5] value")

    # 1D indexing
    c1 = make_const_array(7.0, 10, 1)
    check_close(c1[3], 7.0, "1D c[3]")
    sliced1 = c1[1:4]
    check(isinstance(sliced1, VTKConstantArray),
          "1D c[1:4] should return VTKConstantArray")
    check(sliced1.shape == (3,), f"1D c[1:4] shape: {sliced1.shape}")
    check_close(sliced1.value, 7.0, "1D c[1:4] value")

    # Fancy indexing
    fancy = c1[[1, 4, 5]]
    check(isinstance(fancy, VTKConstantArray),
          "fancy indexing should return VTKConstantArray")
    check(fancy.shape == (3,), f"fancy index shape: {fancy.shape}")
    check_close(fancy.value, 7.0, "fancy index value")

    # Boolean indexing
    mask = np.array([True, False] * 5)
    masked = c1[mask]
    check(isinstance(masked, VTKConstantArray),
          "boolean mask should return VTKConstantArray")
    check(masked.shape == (5,), f"boolean mask shape: {masked.shape}")
    check_close(masked.value, 7.0, "boolean mask value")


# -------------------------------------------------------------------
# Test read-only
# -------------------------------------------------------------------
def test_readonly():
    c = make_const_array(5.0, 10, 1)
    try:
        c[0] = 99.0
        check(False, "__setitem__ should raise TypeError")
    except TypeError:
        pass


# -------------------------------------------------------------------
# Test iteration
# -------------------------------------------------------------------
def test_iteration():
    c1 = make_const_array(4.0, 3, 1)
    vals = list(c1)
    check(len(vals) == 3, f"1D iteration count: {len(vals)}")
    for v in vals:
        check_close(v, 4.0, "1D iteration value")

    c3 = make_const_array(4.0, 3, 3)
    rows = list(c3)
    check(len(rows) == 3, f"multi-comp iteration count: {len(rows)}")
    for row in rows:
        check(row.shape == (3,), f"iteration row shape: {row.shape}")
        check_close(row, np.full(3, 4.0), "iteration row values")


# -------------------------------------------------------------------
# Test convenience methods (sum, mean, min, max, std, var)
# -------------------------------------------------------------------
def test_reductions():
    c = make_const_array(3.0, 10, 1)
    check_close(c.sum(), 30.0, "sum()")
    check_close(c.mean(), 3.0, "mean()")
    check_close(c.min(), 3.0, "min()")
    check_close(c.max(), 3.0, "max()")
    check_close(c.std(), 0.0, "std()")
    check_close(c.var(), 0.0, "var()")

    c3 = make_const_array(2.0, 5, 3)
    check_close(c3.sum(), 30.0, "multi-comp sum()")
    check_close(c3.mean(), 2.0, "multi-comp mean()")


# -------------------------------------------------------------------
# Test astype
# -------------------------------------------------------------------
def test_astype():
    c = make_const_array(5.0, 10, 1)
    c32 = c.astype(np.float32)
    check(isinstance(c32, VTKConstantArray),
          "astype should return VTKConstantArray")
    check(c32.dtype == np.float32, f"astype dtype: {c32.dtype}")
    check_close(c32.value, 5.0, "astype value preserved")
    check(c32.shape == c.shape, "astype shape preserved")


# -------------------------------------------------------------------
# Test repr
# -------------------------------------------------------------------
def test_repr():
    c = make_const_array(5.0, 10, 3)
    r = repr(c)
    check("VTKConstantArray" in r, f"repr contains class name: {r}")
    check("5.0" in r, f"repr contains value: {r}")


# -------------------------------------------------------------------
# Test __array_function__ fallback (e.g. np.concatenate)
# -------------------------------------------------------------------
def test_array_function():
    c = make_const_array(2.0, 3, 1)
    np_arr = np.array([1.0, 2.0, 3.0])

    result = np.concatenate([c, np_arr])
    expected = np.concatenate([np.full(3, 2.0), np_arr])
    check_close(result, expected, "np.concatenate with const array")


# -------------------------------------------------------------------
# Test different dtypes
# -------------------------------------------------------------------
def test_dtypes():
    for dtype in [np.float32, np.float64, np.int32, np.int64]:
        c = make_const_array(7, 5, 1, dtype=dtype)
        np_arr = np.arange(5, dtype=dtype)
        result = c + np_arr
        expected = np.dtype(dtype).type(7) + np_arr
        check_close(result, expected, f"{dtype}: const + np")


# -------------------------------------------------------------------
# Test metadata propagation
# -------------------------------------------------------------------
def test_metadata():
    from vtkmodules.vtkCommonCore import vtkWeakReference
    from vtkmodules.vtkCommonDataModel import vtkPolyData

    ds = vtkPolyData()
    c = make_const_array(5.0, 4, 1)
    c._set_dataset(ds)
    c._association = 0  # POINT

    check(c.dataset is ds, "DataSet should be set")
    check(c.association == 0, "Association should be POINT")

    # const + scalar preserves metadata
    result = c + 2
    check(isinstance(result, VTKConstantArray),
          "const + scalar result type")
    check(result.dataset is ds, "const + scalar should preserve dataset")
    check(result.association == 0, "const + scalar should preserve association")


# -------------------------------------------------------------------
# Test with data_model (dataset integration)
# -------------------------------------------------------------------
def test_dataset_integration():
    from vtkmodules.vtkCommonDataModel import vtkPolyData
    from vtkmodules.vtkCommonCore import vtkPoints

    pd = vtkPolyData()
    pts = vtkPoints()
    pts.InsertNextPoint(0, 0, 0)
    pts.InsertNextPoint(1, 0, 0)
    pts.InsertNextPoint(0, 1, 0)
    pd.SetPoints(pts)

    c = make_const_array(5.0, 3, 1)
    c.SetName("const")
    pd.GetPointData().AddArray(c)

    # Retrieve via data_model
    arr = pd.point_data["const"]
    check(isinstance(arr, VTKConstantArray),
          f"Retrieved array should be VTKConstantArray, got {type(arr)}")
    check(arr.value == 5.0, f"Retrieved value: {arr.value}")
    check(arr.dataset is pd, f"Retrieved dataset should be the polydata")

    # Set via data_model
    c2 = make_const_array(10.0, 3, 1)
    pd.point_data["const2"] = c2
    arr2 = pd.point_data["const2"]
    check(isinstance(arr2, VTKConstantArray),
          "set_array constant should be VTKConstantArray")
    check(arr2.value == 10.0, f"set_array value: {arr2.value}")
    check(arr2.dataset is pd, "set_array dataset should be set")
    # Should be a different object (shallow copy)
    check(arr2 is not c2, "set_array should shallow copy the array")


# -------------------------------------------------------------------
# Test ShallowCopy
# -------------------------------------------------------------------
def test_shallow_copy():
    c = make_const_array(5.0, 10, 3)
    c.SetName("test")

    # NewInstance() on implicit arrays returns vtkAOSDataArrayTemplate
    # (by design in C++), so we create a new instance of the correct type.
    c2 = type(c).__new__(type(c))
    c2.ShallowCopy(c)

    check(isinstance(c2, VTKConstantArray),
          "ShallowCopy result should be VTKConstantArray")
    check(c2.value == 5.0, f"ShallowCopy value: {c2.value}")
    check(c2.shape == (10, 3), f"ShallowCopy shape: {c2.shape}")
    # Note: vtkImplicitArray::ShallowCopy does not copy the name.
    # In set_array, name is set explicitly after ShallowCopy.
    c2.SetName("test")
    check(c2.GetName() == "test", f"ShallowCopy name after SetName: {c2.GetName()}")


# -------------------------------------------------------------------
# Test constructor with value argument
# -------------------------------------------------------------------
def test_constructor_with_shape_and_value():
    # shape as int (single-component)
    c = vtkConstantArray[np.float64](10, 3.14)
    check(isinstance(c, VTKConstantArray),
          "Constructor result should be VTKConstantArray")
    check_close(c.GetConstantValue(), 3.14, "Constructor value")
    check(c.GetNumberOfTuples() == 10, "Constructor ntuples")
    check(c.GetNumberOfComponents() == 1, "Constructor ncomps (default)")

    # shape as tuple (multi-component)
    c2 = vtkConstantArray[np.float64]((5, 3), 7.0)
    check(c2.shape == (5, 3), f"Constructor shape: {c2.shape}")
    check_close(c2.value, 7.0, "Constructor value (tuple shape)")

    # no arguments — valid empty array
    c3 = vtkConstantArray[np.float64]()
    check(c3.GetNumberOfTuples() == 0, "Default constructor ntuples")
    check_close(c3.GetConstantValue(), 0.0, "Default constructor value")


# -------------------------------------------------------------------
# Run all tests
# -------------------------------------------------------------------
test_mixin_applied()
test_properties()
test_array_conversion()
test_arithmetic_with_numpy()
test_arithmetic_with_scalar()
test_multi_component_arithmetic()
test_unary()
test_comparison()
test_ufuncs()
test_const_plus_const()
test_indexing()
test_readonly()
test_iteration()
test_reductions()
test_astype()
test_repr()
test_array_function()
test_dtypes()
test_metadata()
test_dataset_integration()
test_shallow_copy()
test_constructor_with_shape_and_value()

if errors:
    print(f"\n{errors} error(s) found.")
    sys.exit(1)
else:
    print("All tests passed.")
