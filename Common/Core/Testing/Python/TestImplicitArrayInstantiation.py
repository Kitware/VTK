# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
import vtkmodules.vtkCommonCore

import math

NUMERIC_TYPES = [
    'TypeFloat32',
    'TypeFloat64',
    'TypeInt8',
    'TypeInt16',
    'TypeInt32',
    'TypeInt64',
    'TypeUInt8',
    'TypeUInt16',
    'TypeUInt32',
    'TypeUInt64']


# ------------------------------------------------------------------------------
# Get an instance of the corresponding class
def GetArray(backend, dataType):
    vtkClass = 'vtk' + backend + dataType + 'Array'
    return getattr(vtkmodules.vtkCommonCore, vtkClass)()


# ------------------------------------------------------------------------------
# Compare value with tolerance. Print error if different.
def CheckResult(val, expected):
    if not math.isclose(val, expected):
        print("ERROR: has {} instead of {}".format(val, expected))
        return False

    return True


# ------------------------------------------------------------------------------
def TestConstantArray(constArray, value):
    constArray.SetNumberOfTuples(42)
    constArray.ConstructBackend(value)
    if not CheckResult(constArray.GetValue(1), value):
        print("ERROR: {} failed.".format(type(constArray)))


# ------------------------------------------------------------------------------
def TestAffineArray(affineArray, slope, intercept):
    affineArray.SetNumberOfTuples(42)
    affineArray.ConstructBackend(slope, intercept)
    if not CheckResult(affineArray.GetValue(1), slope + intercept):
        print("ERROR: {} failed.".format(type(affineArray)))


# ------------------------------------------------------------------------------
def TestIndexedArray(indexedArray, originalArray):
    indexedArray.SetNumberOfTuples(4)
    indexes = vtkmodules.vtkCommonCore.vtkIdTypeArray()
    size = originalArray.GetNumberOfTuples()
    indexes.InsertNextValue(0)
    indexes.InsertNextValue(1)
    indexes.InsertNextValue(size - 2)
    indexes.InsertNextValue(size - 1)

    indexedArray.ConstructBackend(indexes, originalArray)
    if not CheckResult(indexedArray.GetValue(2), originalArray.GetValue(size - 2)):
        print("ERROR: {} failed.".format(type(indexedArray)))


# ------------------------------------------------------------------------------
def TestCompositeArray(compositeArray, arrayList):
    arrays = vtkmodules.vtkCommonCore.vtkDataArrayCollection()

    firstsize = 13
    first = arrayList[0]
    for idx in range(firstsize):
        first.InsertNextValue(idx)
    arrays.AddItem(first)

    second = arrayList[1]
    secondsize = 14
    for idx in range(firstsize):
        first.InsertNextValue(firstsize + idx)
    arrays.AddItem(second)

    compositeArray.SetNumberOfTuples(firstsize + secondsize)
    compositeArray.ConstructBackend(arrays)

    if not CheckResult(compositeArray.GetValue(firstsize + 1), firstsize + 1):
        print("ERROR: {} failed.".format(type(compositeArray)))


# ------------------------------------------------------------------------------
def InstantiateNumericTypes():
    for dataType in NUMERIC_TYPES:
        constantArray = GetArray("Constant", dataType)
        TestConstantArray(constantArray, 13)

        affineArray = GetArray("Affine", dataType)
        TestAffineArray(affineArray, 1, 2)

        indexedArray = GetArray("Indexed", dataType)
        originalData = GetArray("", dataType)
        for val in range(100):
            originalData.InsertNextValue(val)
        TestIndexedArray(indexedArray, originalData)

        compositeArray = GetArray("Composite", dataType)
        first = GetArray("", dataType)
        second = GetArray("", dataType)
        TestCompositeArray(compositeArray, [first, second])


InstantiateNumericTypes()
