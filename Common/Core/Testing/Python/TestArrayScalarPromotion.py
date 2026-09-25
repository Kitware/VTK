# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
"""Test that arithmetic between VTK arrays and scalars follows numpy.

For every array type, each operator is applied with a scalar on either
side and through the ufunc, and the result (dtype and values, or the
exception raised) is compared with the same operation on the equivalent
numpy array.  numpy treats Python scalars as "weak": float32 + 2 is
float32, int8 + 2 is int8, and int8 + 1000 raises OverflowError.
"""

import operator
import sys

try:
    import numpy as np
except ImportError:
    import vtkmodules.test.Testing
    print("This test requires numpy!")
    vtkmodules.test.Testing.skip()

from vtkmodules.vtkCommonCore import (
    vtkAOSDataArrayTemplate,
    vtkAffineArray,
    vtkCompositeArray,
    vtkConstantArray,
    vtkIdList,
    vtkIndexedArray,
    vtkSOADataArrayTemplate,
    vtkStridedArray,
)
from vtkmodules.vtkCommonDataModel import vtkRectilinearGrid
from vtkmodules.numpy_interface.vtk_partitioned_array import VTKPartitionedArray

errors = 0


def check(condition, msg):
    global errors
    if not condition:
        print("ERROR:", msg)
        errors += 1


# Each operator is checked against the ufunc it stands for rather than the
# ndarray operator: numpy 2.0 - 2.2 square the array for ** 2 without
# promoting, so float32_array ** numpy.float64(2) stays float32 there while
# numpy.power() gives float64.
OPS = {
    "+": (operator.add, np.add),
    "-": (operator.sub, np.subtract),
    "*": (operator.mul, np.multiply),
    "/": (operator.truediv, np.true_divide),
    "//": (operator.floordiv, np.floor_divide),
    "**": (operator.pow, np.power),
    "%": (operator.mod, np.mod),
}

# Structured point arrays don't define //, ** or %.
STRUCTURED_POINT_OPS = ("+", "-", "*", "/")

SCALARS = {
    np.float32: [2, 2.0, np.float64(2)],
    np.int8: [2, 2.0, 1000, np.int16(2)],
}

N = 12


def make_arrays(dtype):
    """Return {name: (vtk_array, operator names to test)} for dtype."""
    base = np.arange(1, 3 * N + 1, dtype=dtype).reshape(N, 3)
    arrays = {}

    arrays["AOS"] = vtkAOSDataArrayTemplate[dtype](base)
    arrays["SOA"] = vtkSOADataArrayTemplate[dtype](
        [np.ascontiguousarray(base[:, i]) for i in range(3)])

    const = vtkConstantArray[dtype]()
    const.ConstructBackend(dtype(2).item())
    const.SetNumberOfComponents(3)
    const.SetNumberOfTuples(N)
    arrays["Constant"] = const

    arrays["Affine"] = vtkAffineArray[dtype](N, 3, 1)

    ids = vtkIdList()
    for i in reversed(range(N)):
        ids.InsertNextId(i)
    indexed = vtkIndexedArray[dtype]()
    indexed.ConstructBackend(ids, arrays["AOS"])
    indexed.SetNumberOfComponents(3)
    indexed.SetNumberOfTuples(N)
    arrays["Indexed"] = indexed

    halves = [vtkAOSDataArrayTemplate[dtype](base[:N // 2]),
              vtkAOSDataArrayTemplate[dtype](base[N // 2:])]
    arrays["Composite"] = vtkCompositeArray[dtype](halves)
    arrays["Partitioned"] = VTKPartitionedArray(
        [vtkAOSDataArrayTemplate[dtype](base[:N // 2]),
         vtkAOSDataArrayTemplate[dtype](base[N // 2:])])

    buf = vtkAOSDataArrayTemplate[dtype](
        np.arange(1, 4 * N + 1, dtype=dtype)).GetBuffer()
    strided = vtkStridedArray[dtype]()
    strided.ConstructBackend(buf, 4, 3, 0)
    arrays["Strided"] = strided

    result = {name: (arr, tuple(OPS)) for name, arr in arrays.items()}

    if dtype == np.float32:
        grid = vtkRectilinearGrid()
        grid.SetDimensions(3, 2, 2)
        grid.SetXCoordinates(vtkAOSDataArrayTemplate[dtype](
            np.array([1, 2, 3], dtype=dtype)))
        grid.SetYCoordinates(vtkAOSDataArrayTemplate[dtype](
            np.array([1, 2], dtype=dtype)))
        grid.SetZCoordinates(vtkAOSDataArrayTemplate[dtype](
            np.array([1, 2], dtype=dtype)))
        result["StructuredPoint"] = (grid.points.data, STRUCTURED_POINT_OPS)

    return result


def outcome(func):
    """Return the result as an ndarray, or the type of the exception."""
    try:
        return np.asarray(func())
    except Exception as e:  # noqa: BLE001
        return type(e)


def compare(name, expression, got, expected):
    if isinstance(expected, type):
        check(got is expected,
              f"{name}: {expression} gave {got}, numpy raises {expected.__name__}")
        return
    if isinstance(got, type):
        check(False, f"{name}: {expression} raised {got.__name__}, "
                     f"numpy gives {expected.dtype}")
        return
    check(got.dtype == expected.dtype,
          f"{name}: {expression} is {got.dtype}, numpy gives {expected.dtype}")
    if got.dtype == expected.dtype and got.shape == expected.shape:
        if np.issubdtype(got.dtype, np.floating):
            same = np.allclose(got, expected, rtol=1e-6)
        else:
            same = np.array_equal(got, expected)
        check(same, f"{name}: {expression} values differ from numpy")


def test_scalar_promotion(dtype):
    for name, (arr, ops) in make_arrays(dtype).items():
        ref = np.array(arr)
        label = f"{name}[{np.dtype(dtype).name}]"
        for s in SCALARS[dtype]:
            for op_name in ops:
                op, ufunc = OPS[op_name]
                compare(label, f"arr {op_name} {s!r}",
                        outcome(lambda: op(arr, s)), outcome(lambda: ufunc(ref, s)))
                compare(label, f"{s!r} {op_name} arr",
                        outcome(lambda: op(s, arr)), outcome(lambda: ufunc(s, ref)))
            compare(label, f"np.add(arr, {s!r})",
                    outcome(lambda: np.add(arr, s)), outcome(lambda: np.add(ref, s)))


def test_constant_with_array():
    """A constant keeps its dtype when combined with a real array."""
    const = vtkConstantArray[np.float64]()
    const.ConstructBackend(2.0)
    const.SetNumberOfTuples(4)
    other = np.ones(4, dtype=np.float32)
    compare("Constant[float64]", "arr + float32 ndarray",
            outcome(lambda: const + other), outcome(lambda: np.array(const) + other))


def test_lazy_results_stay_lazy():
    """Scalar arithmetic on implicit arrays should not materialize them."""
    affine = vtkAffineArray[np.int8](N, 3, 1)
    check(type(affine / 2).__name__.startswith("VTKAffineArray"),
          "affine / 2 should stay an affine array")
    const = vtkConstantArray[np.float32]()
    const.ConstructBackend(2.0)
    const.SetNumberOfTuples(N)
    check(type(const + 2).__name__.startswith("VTKConstantArray"),
          "constant + 2 should stay a constant array")


for dtype in SCALARS:
    test_scalar_promotion(dtype)
test_constant_with_array()
test_lazy_results_stay_lazy()

if errors:
    print(f"\n{errors} error(s) found.")
    sys.exit(1)
else:
    print("All tests passed.")
