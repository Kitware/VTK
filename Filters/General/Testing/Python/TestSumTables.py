#!/usr/bin/env python
import numpy as np

from vtkmodules.vtkCommonCore import vtkDoubleArray, vtkIntArray, vtkUnsignedShortArray, vtkUnsignedIntArray, vtkShortArray
from vtkmodules.vtkCommonDataModel import vtkTable
from vtkmodules.vtkFiltersGeneral import vtkSumTables

from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

aarch64 = False
try:
    import platform
    if platform.machine() == 'aarch64':
        aarch64 = True
except:
    pass

def arrayFromList(arrayType, name, vals):
    array = arrayType()
    array.SetName(name)
    array.SetNumberOfTuples(len(vals))
    ii = 0
    for val in vals:
        array.SetValue(ii, val)
        ii += 1
    return array

def CompareTables(result, expected):
    if result.GetNumberOfColumns() != expected.GetNumberOfColumns():
        print('  ERROR: Column count mismatch')
        return False
    status = True
    for cc in range(result.GetNumberOfColumns()):
        rc = result.GetColumn(cc)
        ec = expected.GetColumnByName(rc.GetName())
        nn = rc.GetNumberOfTuples()
        for ii in range(nn):
            rv = np.array(rc.GetTuple(ii))
            ev = np.array(ec.GetTuple(ii))
            if np.any(rv != ev):
                srv = ' '.join([str(xx) for xx in rv])
                sev = ' '.join([str(xx) for xx in ev])
                print(f'  ERROR: {rc.GetName()} values mismatched at row {ii}: {srv}, expected {sev}')
                status = False
    return status

def SumTableTest(msg, arrays1, arrays2, expected):
    print(f'Testing {msg}.')
    table1 = vtkTable()
    table2 = vtkTable()
    [table1.AddColumn(array) for array in arrays1]
    [table2.AddColumn(array) for array in arrays2]
    sumTables = vtkSumTables()
    sumTables.SetInputDataObject(0, table1)
    sumTables.SetInputDataObject(1, table2)
    sumTables.Update()
    result = sumTables.GetOutputDataObject(0)
    for colName in ('int', 'ush'):
        col = result.GetColumnByName(colName)
        print(f'  Column {colName} is of type {col.GetClassName()}')
    result.Dump(10, -1, 4)
    expectedTable = vtkTable()
    [expectedTable.AddColumn(array) for array in expected]
    if not CompareTables(result, expectedTable):
        result.Dump(10, 10)
        print(f'ERROR: Failed {msg} test.')
        return False
    return True

ok = SumTableTest('columns of same type and signedness',
    (
        arrayFromList(vtkDoubleArray, 'dbl', [-1.5, 0, 2.5, 4.25]),
        arrayFromList(vtkIntArray, 'int', [-1, 0, 3, 5]),
        arrayFromList(vtkUnsignedShortArray, 'ush', [1, 0, 3, 5]),
    ),
    (
        arrayFromList(vtkDoubleArray, 'dbl', [-5.25, 1, 3.5, 1.25]),
        arrayFromList(vtkIntArray, 'int', [-2, 0, 13, 25]),
        arrayFromList(vtkUnsignedShortArray, 'ush', [3, 4, 5, 0]),
    ),
    (
        arrayFromList(vtkDoubleArray, 'dbl', [-6.75, 1, 6, 5.5]),
        arrayFromList(vtkIntArray, 'int', [-3, 0, 16, 30]),
        arrayFromList(vtkUnsignedShortArray, 'ush', [4, 4, 8, 5]),
    )
)
assert(ok)

ok = SumTableTest('columns of differently-signed types',
    (
        arrayFromList(vtkUnsignedIntArray, 'int', [1, 0, 3, 5]),
        arrayFromList(vtkShortArray, 'ush', [1, 0, 3, 5]),
    ),
    (
        arrayFromList(vtkIntArray, 'int', [-2, 0, 13, 25]),
        arrayFromList(vtkUnsignedShortArray, 'ush', [3, 4, 5, 0]),
    ),
    (
        arrayFromList(vtkUnsignedIntArray, 'int', [2**32 - 1, 0, 16, 30]),
        arrayFromList(vtkShortArray, 'ush', [4, 4, 8, 5]),
    )
)
assert(ok)

# Test when the result array is floating-point and the "other" is integral.
# This forces the integer arrays to behave as floating-point data.
ok = SumTableTest('columns of mixed floating and integer type',
    (
        arrayFromList(vtkDoubleArray, 'int', [-2, 0, 13, 25]),
        arrayFromList(vtkDoubleArray, 'ush', [3, 4, 5, 0]),
    ),
    (
        arrayFromList(vtkUnsignedIntArray, 'int', [1, 0, 3, 5]),
        arrayFromList(vtkShortArray, 'ush', [1, 0, 3, 5]),
    ),
    (
        arrayFromList(vtkDoubleArray, 'int', [-1, 0, 16, 30]),
        arrayFromList(vtkDoubleArray, 'ush', [4, 4, 8, 5]),
    )
)
assert(ok)

# Test when the result array is integral and the "other" is floating point.
# This forces the floating-point array to behave as integer data.
#
# Note that this varies on aarch64 vs x86_64/x86/arm64: casting a
# negative floating-point number to an unsigned integer type on aarch64
# yields 0 while on other platforms, it yields a large positive number.
fonky = 0 if aarch64 else 2**32 - 1
print(f'On aarch64? {aarch64} . "Negative" number is {fonky}.')
ok = SumTableTest('columns of mixed integer and floating',
    (
        arrayFromList(vtkUnsignedIntArray, 'int', [1, 0, 3, 5]),
        arrayFromList(vtkShortArray, 'ush', [1, 0, 3, 5]),
    ),
    (
        arrayFromList(vtkDoubleArray, 'int', [-2, 0, 13, 25]),
        arrayFromList(vtkDoubleArray, 'ush', [3, 4, 5, 0]),
    ),
    (
        arrayFromList(vtkUnsignedIntArray, 'int', [fonky, 0, 16, 30]),
        arrayFromList(vtkShortArray, 'ush', [4, 4, 8, 5]),
    )
)
assert(ok)
