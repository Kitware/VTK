# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
"""Test that implicit and partitioned arrays agree with numpy.

Each case compares a VTK array operation with the same operation on the
materialized numpy array: the shape, the dtype and the values, or the
exception raised. Comparing against numpy at run time keeps the test right
under whichever numpy version runs it.
"""

import sys
import warnings

try:
    import numpy as np
except ImportError:
    import vtkmodules.test.Testing
    print("This test requires numpy!")
    vtkmodules.test.Testing.skip()

from vtkmodules.vtkCommonCore import (
    vtkAffineArray,
    vtkConstantArray,
    vtkFloatArray,
    vtkShortArray,
)
from vtkmodules.vtkCommonDataModel import vtkImageData, vtkRectilinearGrid
from vtkmodules.numpy_interface.vtk_partitioned_array import VTKPartitionedArray
from vtkmodules.numpy_interface.vtk_structured_point_array import (
    VTKStructuredPointArray,
)

errors = 0


def check(condition, msg):
    global errors
    if not condition:
        print("ERROR:", msg)
        errors += 1


def outcome(func):
    """Return the result as an ndarray, or the type of the exception."""
    try:
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            return np.asarray(func())
    except Exception as e:  # noqa: BLE001
        return type(e)


def compare(label, got, expected):
    """Compare two outcome() results: shape, dtype and values."""
    if isinstance(expected, type) or isinstance(got, type):
        check(got is expected, f"{label}: got {got}, numpy gives {expected}")
        return
    check(got.shape == expected.shape,
          f"{label}: shape {got.shape}, numpy gives {expected.shape}")
    check(got.dtype == expected.dtype,
          f"{label}: dtype {got.dtype}, numpy gives {expected.dtype}")
    if got.shape == expected.shape:
        check(np.allclose(got, expected, rtol=1e-6, equal_nan=True),
              f"{label}: values differ from numpy")


def image_points():
    image = vtkImageData()
    image.SetDimensions(4, 3, 2)
    image.SetOrigin(-1, 0, 2)
    image.SetSpacing(0.5, 2, 1)
    return image.points.data


def rectilinear_points():
    """Points of a grid with float32 coordinates; the point array is double."""
    grid = vtkRectilinearGrid()
    grid.SetDimensions(3, 2, 2)
    grid.SetXCoordinates(vtkFloatArray(np.array([1, 2, 3], dtype=np.float32)))
    grid.SetYCoordinates(vtkFloatArray(np.array([1, 4], dtype=np.float32)))
    grid.SetZCoordinates(vtkFloatArray(np.array([2, 5], dtype=np.float32)))
    return grid.points.data


def test_structured_points():
    operations = {
        "p * 2 + 1": lambda p: p * 2 + 1,
        "-p": lambda p: -p,
        "abs(p)": abs,
        "np.sin(p)": np.sin,
        "p // 2": lambda p: p // 2,
        "p ** 2": lambda p: p ** 2,
        "p % 2": lambda p: p % 2,
        "2 ** p": lambda p: 2 ** p,
        "5 // p": lambda p: 5 // p,
    }
    for name, make in (("image", image_points), ("rectilinear", rectilinear_points)):
        points = make()
        ref = np.array(points)
        check(np.asarray(points).dtype == points.dtype,
              f"{name}: materialized dtype {np.asarray(points).dtype} "
              f"!= dtype {points.dtype}")
        check(np.asarray(points[:, 0]).dtype == points.dtype,
              f"{name}: column dtype differs from dtype {points.dtype}")
        for label, op in operations.items():
            result = outcome(lambda: op(points))
            compare(f"{name} {label}", result, outcome(lambda: op(ref)))
            if isinstance(result, type):
                continue
            with warnings.catch_warnings():
                warnings.simplefilter("ignore")
                lazy = op(points)
            check(np.asarray(lazy).shape == lazy.shape,
                  f"{name} {label}: .shape {lazy.shape} but values are "
                  f"{np.asarray(lazy).shape}")
        check(isinstance(points * 2 + 1, VTKStructuredPointArray),
              f"{name}: p * 2 + 1 should stay a structured point array")
        check(isinstance(np.sin(points), VTKStructuredPointArray),
              f"{name}: np.sin(p) should stay a structured point array")
        # Comparisons have no lazy form (there is no bool structured-point
        # array) and VTK returns them as int8 arrays, so check values only.
        less = outcome(lambda: points < 2)
        check(not isinstance(less, type) and np.array_equal(less.astype(bool), ref < 2),
              f"{name}: p < 2 gave {less}")


def test_constant_broadcast():
    const = vtkConstantArray[np.float64]((6, 3), 1.5)
    ref = np.array(const)
    others = {
        "arange(3)": np.arange(3.0),
        "ones((6, 1))": np.ones((6, 1)),
        "full-size": np.arange(18.0).reshape(6, 3),
    }
    for label, other in others.items():
        compare(f"const + {label}", outcome(lambda: const + other),
                outcome(lambda: ref + other))
        compare(f"{label} * const", outcome(lambda: other * const),
                outcome(lambda: other * ref))


def test_affine_divide_by_zero():
    affine = vtkAffineArray[np.float64](5, slope=1, intercept=1)
    ref = np.array(affine)
    for label, zero in (("0", 0), ("0.0", 0.0), ("np.float64(0)", np.float64(0))):
        compare(f"affine / {label}", outcome(lambda: affine / zero),
                outcome(lambda: ref / zero))


def test_partitioned_reductions():
    blocks = {
        "float32": lambda: [vtkFloatArray(np.arange(6, dtype=np.float32).reshape(3, 2)),
                            vtkFloatArray(np.arange(6, 10, dtype=np.float32).reshape(2, 2))],
        "int16": lambda: [vtkShortArray(np.arange(6, dtype=np.int16).reshape(3, 2)),
                          vtkShortArray(np.arange(6, 10, dtype=np.int16).reshape(2, 2))],
    }
    functions = (np.sum, np.mean, np.max, np.min, np.var, np.std, np.all, np.average)
    for dtype_name, make in blocks.items():
        partitioned = VTKPartitionedArray(make())
        ref = np.array(partitioned)
        for func in functions:
            for axis in (None, 0):
                compare(f"{func.__name__}(partitioned {dtype_name}, axis={axis})",
                        outcome(lambda: func(partitioned, axis=axis)),
                        outcome(lambda: func(ref, axis=axis)))
        compare(f"partitioned {dtype_name}.max()", outcome(partitioned.max),
                outcome(ref.max))


test_structured_points()
test_constant_broadcast()
test_affine_divide_by_zero()
test_partitioned_reductions()

if errors:
    print(f"\n{errors} error(s) found.")
    sys.exit(1)
else:
    print("All tests passed.")
