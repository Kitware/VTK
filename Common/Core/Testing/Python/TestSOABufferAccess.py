# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
"""Test zero-copy per-component buffer access for vtkSOADataArrayTemplate."""

from vtkmodules.vtkCommonCore import (
    vtkSOADataArrayTemplate,
    vtkFloatArray,
)
from vtkmodules.util.numpy_support import vtk_soa_to_numpy, get_numpy_array_type
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


# -------------------------------------------------------------------
# Test basic SOA buffer access
# -------------------------------------------------------------------
def test_basic_soa():
    a = vtkSOADataArrayTemplate[np.float64]()
    a.SetNumberOfComponents(3)
    a.SetNumberOfTuples(5)

    x = np.array([1.0, 2.0, 3.0, 4.0, 5.0], dtype=np.float64)
    y = np.array([10.0, 20.0, 30.0, 40.0, 50.0], dtype=np.float64)
    z = np.array([100.0, 200.0, 300.0, 400.0, 500.0], dtype=np.float64)

    a.SetArray(0, x, 5, True, True)
    a.SetArray(1, y, 5, True, True)
    a.SetArray(2, z, 5, True, True)

    components = vtk_soa_to_numpy(a)

    check(len(components) == 3, f"Expected 3 components, got {len(components)}")
    check(components[0].shape == (5,), f"Shape: expected (5,), got {components[0].shape}")
    check(components[0].dtype == np.float64, f"dtype: expected float64, got {components[0].dtype}")

    check(np.allclose(components[0], x), f"Component 0 values: {components[0]} != {x}")
    check(np.allclose(components[1], y), f"Component 1 values: {components[1]} != {y}")
    check(np.allclose(components[2], z), f"Component 2 values: {components[2]} != {z}")


# -------------------------------------------------------------------
# Test zero-copy: modification through numpy is visible in VTK
# -------------------------------------------------------------------
def test_zero_copy():
    a = vtkSOADataArrayTemplate[np.float64]()
    a.SetNumberOfComponents(2)
    a.SetNumberOfTuples(4)

    x = np.array([1.0, 2.0, 3.0, 4.0], dtype=np.float64)
    y = np.array([10.0, 20.0, 30.0, 40.0], dtype=np.float64)

    a.SetArray(0, x, 4, True, True)
    a.SetArray(1, y, 4, True, True)

    components = vtk_soa_to_numpy(a)

    # Modify through numpy
    components[0][0] = 999.0

    # Should be visible through VTK
    check(a.GetTypedComponent(0, 0) == 999.0,
          f"Zero-copy failed: VTK sees {a.GetTypedComponent(0, 0)}, expected 999.0")

    # Also verify modification through VTK is visible in numpy
    a.SetTypedComponent(1, 1, 777.0)
    check(components[1][1] == 777.0,
          f"Zero-copy reverse failed: numpy sees {components[1][1]}, expected 777.0")


# -------------------------------------------------------------------
# Test multiple dtypes
# -------------------------------------------------------------------
def test_dtypes():
    for dtype_str in [np.float32, np.float64, np.int32]:
        a = vtkSOADataArrayTemplate[dtype_str]()
        a.SetNumberOfComponents(2)
        a.SetNumberOfTuples(3)

        # Use VTK's own type mapping to ensure exact dtype match across platforms
        vtk_dtype = np.dtype(get_numpy_array_type(a.GetDataType()))

        c0 = np.array([1, 2, 3], dtype=vtk_dtype)
        c1 = np.array([4, 5, 6], dtype=vtk_dtype)

        a.SetArray(0, c0, 3, True, True)
        a.SetArray(1, c1, 3, True, True)

        components = vtk_soa_to_numpy(a)

        check(components[0].dtype == vtk_dtype,
              f"{dtype_str}: expected dtype {vtk_dtype}, got {components[0].dtype}")
        check(np.allclose(components[0], c0),
              f"{dtype_str}: component 0 mismatch")
        check(np.allclose(components[1], c1),
              f"{dtype_str}: component 1 mismatch")


# -------------------------------------------------------------------
# Test single component
# -------------------------------------------------------------------
def test_single_component():
    a = vtkSOADataArrayTemplate[np.float64]()
    a.SetNumberOfComponents(1)
    a.SetNumberOfTuples(4)

    x = np.array([1.0, 2.0, 3.0, 4.0], dtype=np.float64)
    a.SetArray(0, x, 4, True, True)

    components = vtk_soa_to_numpy(a)
    check(len(components) == 1, f"Expected 1 component, got {len(components)}")
    check(np.allclose(components[0], x), "Single component values mismatch")


# -------------------------------------------------------------------
# Test TypeError for non-SOA array
# -------------------------------------------------------------------
def test_type_error():
    a = vtkFloatArray()
    a.SetNumberOfTuples(5)
    a.SetNumberOfComponents(1)

    try:
        vtk_soa_to_numpy(a)
        check(False, "Expected TypeError for vtkFloatArray")
    except TypeError:
        pass  # expected


# -------------------------------------------------------------------
# Test GetComponentBuffer directly - returns vtkAbstractBuffer
# -------------------------------------------------------------------
def test_direct_buffer():
    a = vtkSOADataArrayTemplate[np.float64]()
    a.SetNumberOfComponents(2)
    a.SetNumberOfTuples(3)

    x = np.array([1.0, 2.0, 3.0], dtype=np.float64)
    y = np.array([4.0, 5.0, 6.0], dtype=np.float64)
    a.SetArray(0, x, 3, True, True)
    a.SetArray(1, y, 3, True, True)

    buf = a.GetComponentBuffer(0)
    check(buf is not None, "GetComponentBuffer returned None for SOA array")

    # vtkAbstractBuffer supports the Python buffer protocol, so np.asarray works
    arr = np.asarray(buf)
    check(arr.shape == (3,), f"Buffer shape: expected (3,), got {arr.shape}")
    check(np.allclose(arr, x), f"Buffer values: {arr} != {x}")

    # SetTuple-populated arrays also use SOA storage
    b = vtkSOADataArrayTemplate[np.float64]()
    b.SetNumberOfComponents(1)
    b.SetNumberOfTuples(2)
    b.SetTuple(0, (1.0,))
    b.SetTuple(1, (2.0,))
    buf_st = b.GetComponentBuffer(0)
    check(buf_st is not None, "GetComponentBuffer should work for SetTuple-populated array")


# -------------------------------------------------------------------
# Run all tests
# -------------------------------------------------------------------
test_basic_soa()
test_zero_copy()
test_dtypes()
test_single_component()
test_type_error()
test_direct_buffer()

if errors:
    print(f"\n{errors} error(s) found.")
    sys.exit(1)
else:
    print("All tests passed.")
