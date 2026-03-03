# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
import vtkmodules.vtkCommonCore

import math

VTK_NUMERIC_TYPES = [
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


# ------------------------------------------------------------------------------
# Get an instance of the corresponding class
def GetConcreteArray(backend, dataType):
    vtkClass = 'vtk' + backend + dataType + 'Array'
    return getattr(vtkmodules.vtkCommonCore, vtkClass)()


# ------------------------------------------------------------------------------
# Get an instance of the corresponding template class
def GetTemplateArray(vtkTemplatedArray, dataType):
    return getattr(vtkmodules.vtkCommonCore, vtkTemplatedArray)[dataType]()


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
        second.InsertNextValue(firstsize + idx)
    arrays.AddItem(second)

    compositeArray.SetNumberOfTuples(firstsize + secondsize)
    compositeArray.ConstructBackend(arrays)

    if not CheckResult(compositeArray.GetValue(firstsize + 1), firstsize + 1):
        print("ERROR: {} failed.".format(type(compositeArray)))


# ------------------------------------------------------------------------------
def InstantiateConcreteNumericTypes():
    for dataType in VTK_NUMERIC_TYPES:
        constantArray = GetConcreteArray("Constant", dataType)
        TestConstantArray(constantArray, 13)

        affineArray = GetConcreteArray("Affine", dataType)
        TestAffineArray(affineArray, 1, 2)

        indexedArray = GetConcreteArray("Indexed", dataType)
        originalData = GetConcreteArray("", dataType)
        for val in range(100):
            originalData.InsertNextValue(val)
        TestIndexedArray(indexedArray, originalData)

        compositeArray = GetConcreteArray("Composite", dataType)
        first = GetConcreteArray("", dataType)
        second = GetConcreteArray("", dataType)
        TestCompositeArray(compositeArray, [first, second])


def InstantiateTemplatedNumericTypes():
    for dataType in NUMERIC_TYPES:
        constantArray = GetTemplateArray("vtkConstantArray", dataType)
        TestConstantArray(constantArray, 13)

        affineArray = GetTemplateArray("vtkAffineArray", dataType)
        TestAffineArray(affineArray, 1, 2)

        indexedArray = GetTemplateArray("vtkIndexedArray", dataType)
        originalData = GetTemplateArray("vtkAOSDataArrayTemplate", dataType)
        for val in range(100):
            originalData.InsertNextValue(val)
        TestIndexedArray(indexedArray, originalData)

        compositeArray = GetTemplateArray("vtkCompositeArray", dataType)
        first = GetTemplateArray("vtkAOSDataArrayTemplate", dataType)
        second = GetTemplateArray("vtkAOSDataArrayTemplate", dataType)
        TestCompositeArray(compositeArray, [first, second])


InstantiateConcreteNumericTypes()
InstantiateTemplatedNumericTypes()
