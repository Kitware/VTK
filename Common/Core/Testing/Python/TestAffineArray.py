# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
"""Test vtkAffineArray Python wrapping: bracket instantiation,
GetSlope, GetIntercept, GetComponent, and GetTuple."""

from vtkmodules.vtkCommonCore import vtkAffineArray
import math
import sys

NUMERIC_TYPES = [
    'float32',
    'float64',
    'int8',
    'int16',
    'int32',
    'int64',
    'uint8',
    'uint16',
    'uint32',
    'uint64',
]

errors = 0


def check(condition, msg):
    global errors
    if not condition:
        print("ERROR:", msg)
        errors += 1


def test_affine_array(dtype):
    a = vtkAffineArray[dtype]()
    slope = 3
    intercept = 7
    ntuples = 10
    ncomps = 3

    a.SetNumberOfComponents(ncomps)
    a.SetNumberOfTuples(ntuples)
    a.ConstructBackend(slope, intercept)

    # GetSlope
    check(math.isclose(a.GetSlope(), slope),
          f"{dtype}: GetSlope() expected {slope}, got {a.GetSlope()}")

    # GetIntercept
    check(math.isclose(a.GetIntercept(), intercept),
          f"{dtype}: GetIntercept() expected {intercept}, got {a.GetIntercept()}")

    # GetComponent: value = slope * flat_index + intercept
    for i in range(ntuples):
        for j in range(ncomps):
            flat_index = i * ncomps + j
            expected = slope * flat_index + intercept
            v = a.GetComponent(i, j)
            check(math.isclose(v, expected),
                  f"{dtype}: GetComponent({i}, {j}) expected {expected}, got {v}")

    # GetTuple
    t = a.GetTuple(0)
    for j in range(ncomps):
        expected = slope * j + intercept
        check(math.isclose(t[j], expected),
              f"{dtype}: GetTuple(0)[{j}] expected {expected}, got {t[j]}")


for dtype in NUMERIC_TYPES:
    test_affine_array(dtype)

if errors:
    print(f"\n{errors} error(s) found.")
    sys.exit(1)
else:
    print("All tests passed.")
