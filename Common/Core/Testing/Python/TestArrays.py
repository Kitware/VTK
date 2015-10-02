#!/usr/bin/env python

import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

arrayType = ['Bit', 'Char', 'Double', 'Float', 'Int', 'Long', 'Short',\
 'UnsignedChar', 'UnsignedInt', 'UnsignedLong', 'UnsignedShort']

for array in arrayType:
    print(array + ' array')
    vtkClass = 'vtk' + array + 'Array'
    a = getattr(vtk, vtkClass)()
    a.Allocate(1,1)
    a.SetNumberOfComponents(3)
    a.SetNumberOfTuples(4)

    # InsertComponent
    k = 0
    for i in range(0, a.GetNumberOfTuples()):
        for j in range(0, a.GetNumberOfComponents()):
            a.InsertComponent(i, j, 1)
            k += 1

    # SetComponent
    k = 0
    for i in range(0, a.GetNumberOfTuples()):
        for j in range(0, a.GetNumberOfComponents()):
            a.SetComponent(i, j, 1)
            k += 1

    # DeepCopy
    b = getattr(vtk, vtkClass)()
    b.Allocate(1000, 100)
    # force a resize
    b.InsertComponent(2001, 0, 1)
    b.DeepCopy(a)

    # NewInstance
    m = b.NewInstance()
    m.UnRegister(b)

    # confirm the deep copy
    k = 0
    for i in range(0, a.GetNumberOfTuples()):
        for j in range(0, a.GetNumberOfComponents()):
            if a.GetComponent(i, j) != b.GetComponent(i, j):
                s = '%s: bad component %d, %d' % (array, i, j)
                print(s)
            k += 1

    b.InsertComponent(2001, 0, 1)
    b.Resize(3000)

    a.Squeeze()
    a.Initialize()

    a = None
    b = None

exit()
