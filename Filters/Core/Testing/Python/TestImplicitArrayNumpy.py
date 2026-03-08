# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
"""Tests for VTKImplicitArray fallback mixin.

Implicit arrays whose backends are defined locally in C++ (not wrapped)
cross to Python as vtkDataArray.  The VTKImplicitArray mixin registered
on vtkDataArray gives them a working numpy interface by materialising
via DeepCopy on first access.

This test uses vtkConnectivityFilter which produces an implicit array
for RegionId that is not directly wrapped.
"""

import sys

try:
    import numpy as np
except ImportError:
    import vtkmodules.test.Testing
    print("This test requires numpy!")
    vtkmodules.test.Testing.skip()

from vtkmodules.vtkCommonCore import vtkPoints
from vtkmodules.vtkCommonDataModel import (
    vtkPolyData, vtkTriangle, vtkCellArray,
)
from vtkmodules.vtkFiltersCore import vtkConnectivityFilter
from vtkmodules.numpy_interface.vtk_implicit_array import VTKImplicitArray
from vtkmodules.util.numpy_support import vtk_to_numpy

errors = 0


def check(condition, msg):
    global errors
    if not condition:
        print("ERROR:", msg)
        errors += 1


# ---------------------------------------------------------------------------
# Helper: produce an unwrapped implicit array via vtkConnectivityFilter
# ---------------------------------------------------------------------------
def make_implicit_array():
    """Create a polydata with one triangle, run vtkConnectivityFilter.

    The "RegionId" output array is an implicit array whose backend
    is defined locally — not wrapped for Python.
    """
    points = vtkPoints()
    points.InsertNextPoint(0.0, 0.0, 0.0)
    points.InsertNextPoint(1.0, 0.0, 0.0)
    points.InsertNextPoint(0.0, 1.0, 0.0)

    triangle = vtkTriangle()
    triangle.GetPointIds().SetId(0, 0)
    triangle.GetPointIds().SetId(1, 1)
    triangle.GetPointIds().SetId(2, 2)

    cells = vtkCellArray()
    cells.InsertNextCell(triangle)

    polydata = vtkPolyData()
    polydata.SetPoints(points)
    polydata.SetPolys(cells)

    connectivity = vtkConnectivityFilter()
    connectivity.ColorRegionsOn()
    connectivity.SetInputData(polydata)
    connectivity.Update()

    output = connectivity.GetOutput()
    return output.GetCellData().GetArray("RegionId")


# ===========================================================================
# Tests
# ===========================================================================
def test_isinstance():
    """Unwrapped implicit arrays should be VTKImplicitArray instances."""
    arr = make_implicit_array()
    check(arr is not None, "RegionId array not found")
    check(isinstance(arr, VTKImplicitArray),
          f"Expected VTKImplicitArray, got {type(arr)}")


def test_shape():
    """Shape should reflect number of tuples and components."""
    arr = make_implicit_array()
    # One triangle -> one cell
    check(arr.shape == (1,), f"Expected (1,), got {arr.shape}")
    check(arr.ndim == 1, f"Expected ndim=1, got {arr.ndim}")
    check(len(arr) == 1, f"Expected len=1, got {len(arr)}")


def test_asarray():
    """np.asarray should materialise without error."""
    arr = make_implicit_array()
    result = np.asarray(arr)
    check(isinstance(result, np.ndarray),
          f"Expected ndarray, got {type(result)}")
    check(result.shape == (1,), f"Expected shape (1,), got {result.shape}")


def test_getitem():
    """Indexing should work after materialisation."""
    arr = make_implicit_array()
    val = arr[0]
    # Single connected component -> RegionId should be 0
    check(val == 0, f"Expected 0 for single component, got {val}")


def test_arithmetic():
    """Arithmetic operators should work."""
    arr = make_implicit_array()
    base = np.asarray(arr)[0]

    result = arr + 1
    check(np.asarray(result)[0] == base + 1,
          f"arr + 1 wrong: {np.asarray(result)}")

    result = arr * 2
    check(np.asarray(result)[0] == base * 2,
          f"arr * 2 wrong: {np.asarray(result)}")


def test_comparison():
    """Comparison operators should work."""
    arr = make_implicit_array()
    result = arr == 0
    check(np.asarray(result)[0] == True,
          f"arr == 0 should be True, got {np.asarray(result)}")


def test_reductions():
    """numpy reductions should work."""
    arr = make_implicit_array()
    s = np.sum(arr)
    check(s == 0, f"sum = {s}, expected 0")

    m = np.mean(arr)
    check(m == 0.0, f"mean = {m}, expected 0.0")


def test_ufuncs():
    """numpy ufuncs should work."""
    arr = make_implicit_array()
    result = np.abs(arr)
    check(np.asarray(result)[0] == 0,
          f"abs wrong: {np.asarray(result)}")


def test_repr():
    """repr should be informative."""
    arr = make_implicit_array()
    r = repr(arr)
    check("VTKImplicitArray" in r, f"repr wrong: {r}")


def test_iter():
    """Iteration should work."""
    arr = make_implicit_array()
    values = list(arr)
    check(len(values) == 1, f"Expected 1 value, got {len(values)}")


def test_radd():
    """Reverse arithmetic should work."""
    arr = make_implicit_array()
    base = np.asarray(arr)[0]
    result = 10 + arr
    check(np.asarray(result)[0] == 10 + base,
          f"10 + arr wrong: {np.asarray(result)}")


def test_unary():
    """Unary operators should work."""
    arr = make_implicit_array()
    result = -arr
    check(np.asarray(result)[0] == -np.asarray(arr)[0],
          f"-arr wrong: {np.asarray(result)}")


def test_vtk_to_numpy():
    """vtk_to_numpy should work on implicit arrays via the buffer protocol."""
    arr = make_implicit_array()
    result = vtk_to_numpy(arr)
    check(isinstance(result, np.ndarray),
          f"vtk_to_numpy should return ndarray, got {type(result)}")
    check(result.shape == (1,), f"Expected shape (1,), got {result.shape}")
    check(result[0] == 0, f"Expected 0 for single component, got {result[0]}")


def test_numpy_functions():
    """Various numpy functions should work via __array_function__."""
    arr = make_implicit_array()

    result = np.sort(arr)
    check(isinstance(result, np.ndarray), "sort should return ndarray")

    result = np.clip(arr, -1, 1)
    check(isinstance(result, np.ndarray), "clip should return ndarray")


# ===========================================================================
# Run all tests
# ===========================================================================
test_isinstance()
test_shape()
test_asarray()
test_getitem()
test_arithmetic()
test_comparison()
test_reductions()
test_ufuncs()
test_repr()
test_iter()
test_radd()
test_unary()
test_vtk_to_numpy()
test_numpy_functions()

if errors:
    print(f"\n{errors} error(s) found!")
    sys.exit(1)
else:
    print("\nAll implicit array tests passed!")
