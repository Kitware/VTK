# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
"""Test vtkConstantArray Python wrapping: bracket instantiation,
GetComponent, GetTypedTuple, and GetConstantValue."""

from vtkmodules.vtkCommonCore import vtkConstantArray
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


def test_constant_array(dtype):
    a = vtkConstantArray[dtype]()
    value = 42
    ntuples = 10
    ncomps = 3

    a.SetNumberOfComponents(ncomps)
    a.SetNumberOfTuples(ntuples)
    a.ConstructBackend(value)

    # GetConstantValue
    check(math.isclose(a.GetConstantValue(), value),
          f"{dtype}: GetConstantValue() expected {value}, got {a.GetConstantValue()}")

    # GetComponent
    for i in range(ntuples):
        for j in range(ncomps):
            v = a.GetComponent(i, j)
            check(math.isclose(v, value),
                  f"{dtype}: GetComponent({i}, {j}) expected {value}, got {v}")

    # GetTuple
    t = a.GetTuple(0)
    for j in range(ncomps):
        check(math.isclose(t[j], value),
              f"{dtype}: GetTuple(0)[{j}] expected {value}, got {t[j]}")


for dtype in NUMERIC_TYPES:
    test_constant_array(dtype)

if errors:
    print(f"\n{errors} error(s) found.")
    sys.exit(1)
else:
    print("All tests passed.")
