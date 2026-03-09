# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
"""Comprehensive tests for VTKAOSArray mixin."""

from vtkmodules.vtkCommonCore import (
    vtkFloatArray, vtkDoubleArray,
    vtkIntArray, vtkUnsignedIntArray,
    vtkShortArray, vtkUnsignedShortArray,
    vtkSignedCharArray, vtkUnsignedCharArray,
    vtkLongLongArray, vtkUnsignedLongLongArray,
    vtkIdTypeArray,
    vtkDataArray,
)
import sys

try:
    import numpy as np
except ImportError:
    import vtkmodules.test.Testing
    print("This test requires numpy!")
    vtkmodules.test.Testing.skip()

from vtkmodules.numpy_interface.vtk_aos_array import VTKAOSArray
from vtkmodules.util.numpy_support import numpy_to_vtk, vtk_to_numpy

errors = 0


def check(condition, msg):
    global errors
    if not condition:
        print("ERROR:", msg)
        errors += 1


def make_aos(n_tuples=5, n_comps=3, dtype=np.float64):
    """Helper: create an AOS array with known data."""
    data = np.arange(n_tuples * n_comps, dtype=dtype).reshape(n_tuples, n_comps)
    vtk_arr = numpy_to_vtk(data)
    vtk_arr.SetName("test")
    return vtk_arr, data


def make_aos_1comp(n_tuples=10, dtype=np.float64):
    """Helper: create a 1-component AOS array."""
    data = np.arange(n_tuples, dtype=dtype)
    vtk_arr = numpy_to_vtk(data)
    vtk_arr.SetName("test1d")
    return vtk_arr, data


# -------------------------------------------------------------------
# 1. Override registration — concrete types
# -------------------------------------------------------------------
def test_override_concrete():
    concrete_classes = [
        vtkFloatArray, vtkDoubleArray,
        vtkIntArray, vtkUnsignedIntArray,
        vtkShortArray, vtkUnsignedShortArray,
        vtkSignedCharArray, vtkUnsignedCharArray,
        vtkLongLongArray, vtkUnsignedLongLongArray,
        vtkIdTypeArray,
    ]
    for cls in concrete_classes:
        a = cls()
        check(isinstance(a, VTKAOSArray),
              f"{cls.__name__}() should be VTKAOSArray instance, got {type(a).__name__}")
        check(isinstance(a, vtkDataArray),
              f"{cls.__name__}() should still be a vtkDataArray")
    print("  test_override_concrete PASSED")

test_override_concrete()


# -------------------------------------------------------------------
# 2. Template access
# -------------------------------------------------------------------
def test_template_access():
    from vtkmodules.vtkCommonCore import vtkAOSDataArrayTemplate
    for dt in [np.float32, np.float64, np.int32, np.int64]:
        a = vtkAOSDataArrayTemplate[dt]()
        check(isinstance(a, VTKAOSArray),
              f"vtkAOSDataArrayTemplate[{dt}]() should be VTKAOSArray")
    print("  test_template_access PASSED")

test_template_access()


# -------------------------------------------------------------------
# 3. Properties — shape, dtype, ndim, size
# -------------------------------------------------------------------
def test_properties():
    vtk_arr, data = make_aos(5, 3)
    check(vtk_arr.shape == (5, 3), f"shape should be (5, 3), got {vtk_arr.shape}")
    check(vtk_arr.ndim == 2, f"ndim should be 2, got {vtk_arr.ndim}")
    check(vtk_arr.size == 15, f"size should be 15, got {vtk_arr.size}")
    check(vtk_arr.dtype == np.float64, f"dtype should be float64, got {vtk_arr.dtype}")

    vtk_arr1, data1 = make_aos_1comp(10)
    check(vtk_arr1.shape == (10,), f"1-comp shape should be (10,), got {vtk_arr1.shape}")
    check(vtk_arr1.ndim == 1, f"1-comp ndim should be 1, got {vtk_arr1.ndim}")
    check(vtk_arr1.size == 10, f"1-comp size should be 10, got {vtk_arr1.size}")
    print("  test_properties PASSED")

test_properties()


# -------------------------------------------------------------------
# 4. __array__ — correctness and zero-copy
# -------------------------------------------------------------------
def test_array_protocol():
    vtk_arr, data = make_aos(5, 3)
    arr = np.array(vtk_arr)
    check(np.array_equal(arr, data), "__array__ data mismatch")

    # Check that np.asarray returns a view (zero-copy)
    view = np.asarray(vtk_arr)
    check(np.array_equal(view, data), "np.asarray data mismatch")
    print("  test_array_protocol PASSED")

test_array_protocol()


# -------------------------------------------------------------------
# 5. Zero-copy — mutations visible both directions
# -------------------------------------------------------------------
def test_zero_copy():
    vtk_arr, data = make_aos(5, 3)
    view = vtk_arr._array_view
    check(view is not None, "array view should not be None")

    # Modify through numpy view
    old_val = view[2, 1]
    view[2, 1] = 999.0
    check(vtk_arr.GetComponent(2, 1) == 999.0,
          f"VTK should see numpy modification, got {vtk_arr.GetComponent(2, 1)}")

    # Modify through VTK
    vtk_arr.SetComponent(0, 0, -42.0)
    check(view[0, 0] == -42.0,
          f"numpy should see VTK modification, got {view[0, 0]}")
    print("  test_zero_copy PASSED")

test_zero_copy()


# -------------------------------------------------------------------
# 6. Arithmetic — scalar ops, array+array, broadcasting
# -------------------------------------------------------------------
def test_arithmetic():
    vtk_arr, data = make_aos(5, 3)

    # scalar
    result = vtk_arr + 1
    check(np.array_equal(result, data + 1), "scalar add failed")
    result = vtk_arr * 2
    check(np.array_equal(result, data * 2), "scalar mul failed")
    result = vtk_arr - 1
    check(np.array_equal(result, data - 1), "scalar sub failed")
    result = vtk_arr / 2
    check(np.allclose(result, data / 2), "scalar div failed")

    # array + array
    vtk_arr2, data2 = make_aos(5, 3)
    result = vtk_arr + vtk_arr2
    check(np.array_equal(result, data + data2), "array+array failed")

    # broadcasting (n,3) + (n,)  — reshape_for_broadcast adds trailing dim
    vtk_1d, data_1d = make_aos_1comp(5)
    result = vtk_arr + vtk_1d
    expected = data + data_1d.reshape(-1, 1)
    check(np.array_equal(result, expected),
          f"broadcast (n,3)+(n,) failed: {result} vs {expected}")

    # reverse ops
    result = 10 - vtk_arr
    check(np.array_equal(result, 10 - data), "rsub failed")
    result = 2 * vtk_arr
    check(np.array_equal(result, 2 * data), "rmul failed")

    print("  test_arithmetic PASSED")

test_arithmetic()


# -------------------------------------------------------------------
# 7. Comparison operators
# -------------------------------------------------------------------
def test_comparison():
    vtk_arr, data = make_aos(5, 3)
    check(np.array_equal(vtk_arr > 5, data > 5), "__gt__ failed")
    check(np.array_equal(vtk_arr < 5, data < 5), "__lt__ failed")
    check(np.array_equal(vtk_arr >= 5, data >= 5), "__ge__ failed")
    check(np.array_equal(vtk_arr <= 5, data <= 5), "__le__ failed")
    check(np.array_equal(vtk_arr == 5, data == 5), "__eq__ failed")
    check(np.array_equal(vtk_arr != 5, data != 5), "__ne__ failed")
    print("  test_comparison PASSED")

test_comparison()


# -------------------------------------------------------------------
# 8. Ufuncs — np.sin, np.sqrt, np.add
# -------------------------------------------------------------------
def test_ufuncs():
    vtk_arr, data = make_aos(5, 3, dtype=np.float64)
    # Offset data to avoid sqrt of 0 issues
    data_shifted = data + 1.0

    vtk_shifted = numpy_to_vtk(data_shifted)
    check(np.allclose(np.sin(vtk_shifted), np.sin(data_shifted)), "np.sin failed")
    check(np.allclose(np.sqrt(vtk_shifted), np.sqrt(data_shifted)), "np.sqrt failed")

    vtk_arr2, data2 = make_aos(5, 3, dtype=np.float64)
    result = np.add(vtk_arr, vtk_arr2)
    check(np.array_equal(result, np.add(data, data2)), "np.add failed")
    print("  test_ufuncs PASSED")

test_ufuncs()


# -------------------------------------------------------------------
# 9. Indexing — element, slice, column, fancy, boolean
# -------------------------------------------------------------------
def test_indexing():
    vtk_arr, data = make_aos(5, 3)

    # Element
    check(np.array_equal(vtk_arr[0], data[0]), "element index failed")
    check(np.array_equal(vtk_arr[-1], data[-1]), "negative element index failed")

    # Slice
    check(np.array_equal(vtk_arr[1:3], data[1:3]), "slice failed")

    # Column
    check(np.array_equal(vtk_arr[:, 0], data[:, 0]), "column index failed")
    check(np.array_equal(vtk_arr[:, -1], data[:, -1]), "negative column index failed")

    # Fancy indexing
    idx = [0, 2, 4]
    check(np.array_equal(vtk_arr[idx], data[idx]), "fancy index failed")

    # Boolean indexing
    mask = np.array([True, False, True, False, True])
    check(np.array_equal(vtk_arr[mask], data[mask]), "boolean index failed")

    # 1-component indexing
    vtk_1d, data_1d = make_aos_1comp(10)
    check(vtk_1d[3] == data_1d[3], "1-comp element index failed")
    check(np.array_equal(vtk_1d[2:5], data_1d[2:5]), "1-comp slice failed")

    print("  test_indexing PASSED")

test_indexing()


# -------------------------------------------------------------------
# 10. Setitem — element, slice, column
# -------------------------------------------------------------------
def test_setitem():
    vtk_arr, data = make_aos(5, 3)

    vtk_arr[0] = [100, 200, 300]
    check(vtk_arr.GetComponent(0, 0) == 100, "setitem element comp 0 failed")
    check(vtk_arr.GetComponent(0, 1) == 200, "setitem element comp 1 failed")
    check(vtk_arr.GetComponent(0, 2) == 300, "setitem element comp 2 failed")

    vtk_arr[1:3] = 42
    view = vtk_arr._array_view
    check(view[1, 0] == 42 and view[2, 2] == 42, "setitem slice failed")

    vtk_arr[:, 0] = -1
    check(view[0, 0] == -1 and view[4, 0] == -1, "setitem column failed")

    print("  test_setitem PASSED")

test_setitem()


# -------------------------------------------------------------------
# 11. Reductions — sum, mean, min, max with axis
# -------------------------------------------------------------------
def test_reductions():
    vtk_arr, data = make_aos(5, 3, dtype=np.float64)

    check(np.isclose(vtk_arr.sum(), data.sum()), f"sum() failed: {vtk_arr.sum()} vs {data.sum()}")
    check(np.allclose(vtk_arr.sum(axis=0), data.sum(axis=0)), "sum(axis=0) failed")
    check(np.allclose(vtk_arr.sum(axis=1), data.sum(axis=1)), "sum(axis=1) failed")

    check(np.isclose(vtk_arr.mean(), data.mean()), "mean() failed")
    check(np.allclose(vtk_arr.mean(axis=0), data.mean(axis=0)), "mean(axis=0) failed")

    check(vtk_arr.min() == data.min(), "min() failed")
    check(np.array_equal(vtk_arr.min(axis=0), data.min(axis=0)), "min(axis=0) failed")

    check(vtk_arr.max() == data.max(), "max() failed")
    check(np.array_equal(vtk_arr.max(axis=0), data.max(axis=0)), "max(axis=0) failed")

    print("  test_reductions PASSED")

test_reductions()


# -------------------------------------------------------------------
# 12. Memory safety — BufferChangedEvent invalidates cache
# -------------------------------------------------------------------
def test_memory_safety():
    from vtkmodules.numpy_interface.vtk_aos_array import _UNINITIALIZED

    vtk_arr = vtkFloatArray()
    vtk_arr.SetNumberOfComponents(3)
    vtk_arr.SetNumberOfTuples(5)
    for i in range(5):
        for c in range(3):
            vtk_arr.SetComponent(i, c, float(i * 3 + c))

    # Force cache init
    view1 = vtk_arr._array_view
    check(view1 is not None, "initial view should not be None")
    check(view1.shape == (5, 3), f"initial shape should be (5, 3), got {view1.shape}")

    # ReserveTuples to much larger (triggers BufferChangedEvent via ReallocateTuples)
    vtk_arr.ReserveTuples(1000)
    vtk_arr.SetNumberOfTuples(1000)

    # Cache should be invalidated
    check(vtk_arr._array_cache is _UNINITIALIZED,
          "cache should be UNINITIALIZED after resize")

    # New view should work with new size
    for i in range(1000):
        for c in range(3):
            vtk_arr.SetComponent(i, c, float(i * 3 + c))
    view2 = vtk_arr._array_view
    check(view2 is not None, "new view should not be None after resize")
    check(view2.shape == (1000, 3), f"new view shape should be (1000, 3), got {view2.shape}")
    check(view2[999, 2] == 2999.0, f"new view data should be correct, got {view2[999, 2]}")

    print("  test_memory_safety PASSED")

test_memory_safety()


# -------------------------------------------------------------------
# 13. Metadata — _dataset, _association via data_model
# -------------------------------------------------------------------
def test_metadata():
    from vtkmodules.vtkCommonDataModel import vtkPolyData, vtkDataObject

    pd = vtkPolyData()
    pd.GetPointData().SetNumberOfTuples(5)

    vtk_arr, data = make_aos(5, 3)
    vtk_arr.SetName("coords")
    pd.GetPointData().AddArray(vtk_arr)

    # Access through data_model
    arr = pd.point_data["coords"]
    check(isinstance(arr, VTKAOSArray), f"Should get VTKAOSArray from data_model, got {type(arr).__name__}")
    check(arr._association == vtkDataObject.POINT,
          f"association should be POINT, got {arr._association}")
    check(arr._dataset is not None, "_dataset should be set")

    print("  test_metadata PASSED")

test_metadata()


# -------------------------------------------------------------------
# 14. _from_array — round-trip numpy→VTK→numpy
# -------------------------------------------------------------------
def test_from_array():
    data = np.array([[1.0, 2.0, 3.0], [4.0, 5.0, 6.0]], dtype=np.float64)
    vtk_arr = VTKAOSArray._from_array(data)
    check(isinstance(vtk_arr, VTKAOSArray),
          f"_from_array should return VTKAOSArray, got {type(vtk_arr).__name__}")
    result = np.array(vtk_arr)
    check(np.array_equal(result, data), "_from_array round-trip failed")
    print("  test_from_array PASSED")

test_from_array()


# -------------------------------------------------------------------
# 15. Multiple dtypes — float32, float64, int32, int64
# -------------------------------------------------------------------
def test_dtypes():
    for dtype in [np.float32, np.float64, np.int32, np.int64]:
        vtk_arr, data = make_aos(5, 3, dtype=dtype)
        result = np.array(vtk_arr)
        check(result.dtype == dtype, f"dtype mismatch for {dtype}: got {result.dtype}")
        check(np.array_equal(result, data), f"data mismatch for {dtype}")
    print("  test_dtypes PASSED")

test_dtypes()


# -------------------------------------------------------------------
# 16. Empty arrays — 0 tuples, populate later
# -------------------------------------------------------------------
def test_empty():
    a = vtkFloatArray()
    a.SetNumberOfComponents(3)
    check(isinstance(a, VTKAOSArray), "empty array should be VTKAOSArray")
    check(a.shape == (0, 3), f"empty shape should be (0, 3), got {a.shape}")
    check(len(a) == 0, "empty len should be 0")

    arr = np.array(a)
    check(arr.shape == (0, 3), f"empty np.array shape should be (0, 3), got {arr.shape}")

    # Populate later
    a.SetNumberOfTuples(3)
    for i in range(3):
        for c in range(3):
            a.SetComponent(i, c, float(i * 3 + c))

    view = a._array_view
    check(view is not None, "view should work after populating empty array")
    check(view.shape == (3, 3), f"shape should be (3, 3) after populate, got {view.shape}")
    check(view[2, 2] == 8.0, f"data should be correct after populate, got {view[2, 2]}")
    print("  test_empty PASSED")

test_empty()


# -------------------------------------------------------------------
# 17. VTK method access — GetName, SetName, GetNumberOfTuples
# -------------------------------------------------------------------
def test_vtk_methods():
    vtk_arr, data = make_aos(5, 3)
    vtk_arr.SetName("my_array")
    check(vtk_arr.GetName() == "my_array", f"GetName failed: {vtk_arr.GetName()}")
    check(vtk_arr.GetNumberOfTuples() == 5,
          f"GetNumberOfTuples failed: {vtk_arr.GetNumberOfTuples()}")
    check(vtk_arr.GetNumberOfComponents() == 3,
          f"GetNumberOfComponents failed: {vtk_arr.GetNumberOfComponents()}")
    print("  test_vtk_methods PASSED")

test_vtk_methods()


# -------------------------------------------------------------------
# 18. Unary operators
# -------------------------------------------------------------------
def test_unary():
    vtk_arr, data = make_aos(5, 3, dtype=np.float64)
    check(np.array_equal(-vtk_arr, -data), "__neg__ failed")
    check(np.array_equal(abs(vtk_arr), np.abs(data)), "__abs__ failed")
    print("  test_unary PASSED")

test_unary()


# -------------------------------------------------------------------
# 19. Shape / layout methods
# -------------------------------------------------------------------
def test_shape_methods():
    vtk_arr, data = make_aos(5, 3)
    check(np.array_equal(vtk_arr.flatten(), data.flatten()), "flatten failed")
    check(np.array_equal(vtk_arr.ravel(), data.ravel()), "ravel failed")
    check(np.array_equal(vtk_arr.reshape(15), data.reshape(15)), "reshape failed")
    check(vtk_arr.tolist() == data.tolist(), "tolist failed")
    check(np.array_equal(vtk_arr.T, data.T), "T failed")
    print("  test_shape_methods PASSED")

test_shape_methods()


# -------------------------------------------------------------------
# 20. __len__, __iter__, __repr__, __str__
# -------------------------------------------------------------------
def test_utils():
    vtk_arr, data = make_aos(5, 3)
    check(len(vtk_arr) == 5, f"__len__ failed: {len(vtk_arr)}")

    items = list(vtk_arr)
    check(len(items) == 5, f"__iter__ length failed: {len(items)}")
    check(np.array_equal(items[0], data[0]), "__iter__ data failed")

    r = repr(vtk_arr)
    check("VTKAOSArray" in r, f"__repr__ should contain VTKAOSArray: {r}")

    s = str(vtk_arr)
    check(len(s) > 0, "__str__ should not be empty")
    print("  test_utils PASSED")

test_utils()


# -------------------------------------------------------------------
# 21. data_model access returns VTKAOSArray
# -------------------------------------------------------------------
def test_data_model_access():
    from vtkmodules.vtkCommonDataModel import vtkPolyData

    pd = vtkPolyData()
    pd.GetPointData().SetNumberOfTuples(5)

    vtk_arr = vtkFloatArray()
    vtk_arr.SetName("test_dm")
    vtk_arr.SetNumberOfComponents(3)
    vtk_arr.SetNumberOfTuples(5)
    for i in range(15):
        vtk_arr.SetComponent(i // 3, i % 3, float(i))
    pd.GetPointData().AddArray(vtk_arr)

    # Access via data_model returns VTKAOSArray directly
    arr = pd.point_data["test_dm"]
    check(isinstance(arr, VTKAOSArray),
          f"data_model access should return VTKAOSArray, got {type(arr).__name__}")
    check(np.array_equal(np.array(arr), np.arange(15, dtype=np.float32).reshape(5, 3)),
          "data_model access data mismatch")
    print("  test_data_model_access PASSED")

test_data_model_access()


# -------------------------------------------------------------------
# 22. astype
# -------------------------------------------------------------------
def test_astype():
    vtk_arr, data = make_aos(5, 3, dtype=np.float64)
    result = vtk_arr.astype(np.float32)
    check(result.dtype == np.float32, f"astype dtype should be float32, got {result.dtype}")
    check(np.allclose(result, data.astype(np.float32)), "astype data mismatch")
    print("  test_astype PASSED")

test_astype()


# -------------------------------------------------------------------
# 23. clip and round
# -------------------------------------------------------------------
def test_clip_round():
    vtk_arr, data = make_aos(5, 3, dtype=np.float64)
    clipped = vtk_arr.clip(2.0, 10.0)
    check(np.array_equal(clipped, data.clip(2.0, 10.0)), "clip failed")

    rounded = vtk_arr.round(0)
    check(np.array_equal(rounded, data.round(0)), "round failed")
    print("  test_clip_round PASSED")

test_clip_round()


# -------------------------------------------------------------------
# 24. nbytes
# -------------------------------------------------------------------
def test_nbytes():
    vtk_arr, data = make_aos(5, 3, dtype=np.float64)
    check(vtk_arr.nbytes == data.nbytes, f"nbytes mismatch: {vtk_arr.nbytes} vs {data.nbytes}")
    print("  test_nbytes PASSED")

test_nbytes()


# -------------------------------------------------------------------
# 25. Metadata propagation through element-wise operations
# -------------------------------------------------------------------
def test_metadata_propagation():
    from vtkmodules.vtkCommonDataModel import vtkPolyData, vtkDataObject

    pd = vtkPolyData()
    pd.GetPointData().SetNumberOfTuples(5)

    vtk_arr, data = make_aos(5, 3)
    vtk_arr.SetName("coords")
    pd.GetPointData().AddArray(vtk_arr)

    arr = pd.point_data["coords"]
    check(arr._dataset is not None, "metadata_propagation: _dataset should be set")
    check(arr._association == vtkDataObject.POINT,
          "metadata_propagation: _association should be POINT")

    # Arithmetic: arr + 1
    result = arr + 1
    check(isinstance(result, VTKAOSArray),
          f"arr+1 should be VTKAOSArray, got {type(result).__name__}")
    check(result.dataset is not None, "arr+1 should have DataSet")
    check(result.association == vtkDataObject.POINT, "arr+1 should have Association")

    # Ufunc: np.sin(arr)
    result = np.sin(arr)
    check(isinstance(result, VTKAOSArray),
          f"np.sin(arr) should be VTKAOSArray, got {type(result).__name__}")
    check(result.dataset is not None, "np.sin(arr) should have DataSet")
    check(result.association == vtkDataObject.POINT, "np.sin(arr) should have Association")

    # Unary: -arr
    result = -arr
    check(isinstance(result, VTKAOSArray),
          f"-arr should be VTKAOSArray, got {type(result).__name__}")
    check(result.dataset is not None, "-arr should have DataSet")
    check(result.association == vtkDataObject.POINT, "-arr should have Association")

    # Unary: abs(arr)
    result = abs(arr)
    check(isinstance(result, VTKAOSArray),
          f"abs(arr) should be VTKAOSArray, got {type(result).__name__}")
    check(result.dataset is not None, "abs(arr) should have DataSet")
    check(result.association == vtkDataObject.POINT, "abs(arr) should have Association")

    # Comparison: arr > 0
    result = arr > 0
    check(isinstance(result, VTKAOSArray),
          f"arr>0 should be VTKAOSArray, got {type(result).__name__}")
    check(result.dataset is not None, "arr>0 should have DataSet")
    check(result.association == vtkDataObject.POINT, "arr>0 should have Association")

    # Reductions should NOT return VTKAOSArray
    result = arr.sum()
    check(not isinstance(result, VTKAOSArray),
          "arr.sum() should be plain scalar, not VTKAOSArray")

    result = arr.sum(axis=0)
    check(not isinstance(result, VTKAOSArray),
          "arr.sum(axis=0) should be plain ndarray, not VTKAOSArray")

    print("  test_metadata_propagation PASSED")

test_metadata_propagation()


# -------------------------------------------------------------------
# Done
# -------------------------------------------------------------------
if errors:
    print(f"\n{errors} error(s) found!")
    sys.exit(1)
else:
    print("\nAll tests passed!")
