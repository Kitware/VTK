#!/usr/bin/env python

import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# types can be specified either with one of the strings listed below,
# or with a numpy dtype.  You can get a list of types by calling the
# method vtkSOADataArrayTemplate.keys().
types = ['char', 'int8', 'uint8', 'int16', 'uint16', 'int32', 'uint32',
         'int64', 'uint64', 'float32', 'float64']

for T in types:
    # instantiate the class for type T
    a = vtk.vtkSOADataArrayTemplate[T]()
    a.SetNumberOfComponents(3)
    # do a simple set/get test
    a.SetNumberOfTuples(2)
    a.SetTuple(0, (1,2,3))
    assert a.GetTuple(0) == (1,2,3)
    # test the typed interface
    if T != 'char':
        a.SetTypedTuple(0, (3,4,6))
        t = [0,0,0]
        a.GetTypedTuple(0, t)
        assert t == [3,4,6]
        assert a.GetTypedComponent(0, 0) == 3
        assert a.GetTypedComponent(0, 1) == 4
        assert a.GetTypedComponent(0, 2) == 6

exit()
