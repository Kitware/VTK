#!/usr/bin/env python

import array
import vtk

# types can be specified either with one of the strings listed below,
# or with a numpy dtype.  You can get a list of types by calling the
# method vtkSOADataArrayTemplate.keys().
types = ['char', 'int8', 'uint8', 'int16', 'uint16', 'int32', 'uint32',
         'int64', 'uint64', 'float32', 'float64']
formats = ['c', 'b', 'B', 'h', 'H', 'i', 'I', 'q', 'Q', 'f', 'd']

numarraytypes = 1
if hasattr(vtk, "vtkScaledSOADataArrayTemplate"):
    numarraytypes = 2

for i in range(numarraytypes):
    for T in types:
        # instantiate the class for type T
        if i == 0:
            a = vtk.vtkSOADataArrayTemplate[T]()
        else:
            a = vtk.vtkScaledSOADataArrayTemplate[T]()
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
        # test SetArray for types compatible with 'array' module
        if T not in ('char', 'int64', 'uint64'):
            t = formats[types.index(T)]
            b1 = array.array(t, [10, 11, 12, 20])
            b2 = array.array(t, [14, 15, 16, 30])
            b3 = array.array(t, [17, 18, 19, 40])
            a.SetArray(0, b1, 4, True, True)
            a.SetArray(1, b2, 4, True, True)
            a.SetArray(2, b3, 4, True, True)
            assert a.GetNumberOfTuples() == 4
            assert a.GetTuple(0) == (10, 14, 17)
            assert a.GetTuple(1) == (11, 15, 18)
            assert a.GetTuple(2) == (12, 16, 19)
            assert a.GetTuple(3) == (20, 30, 40)

exit()
