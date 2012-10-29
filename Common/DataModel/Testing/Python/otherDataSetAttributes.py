#!/usr/bin/env python
import vtk
dsa = vtk.vtkDataSetAttributes()
for array in "Bit Char Double Float Int Long Short UnsignedChar UnsignedInt UnsignedLong UnsignedShort".split():

    var =  eval('vtk.vtk'+ array +'Array')()
    var.Allocate(1,1)
    var.SetNumberOfComponents(3)
    var.SetNumberOfTuples(4)
    var.SetName("a"+array+"Array")

    # SetComponent
    k = 0
    i = 0
    while i < var.GetNumberOfTuples():
        j = 0
        while j < var.GetNumberOfComponents():
            var.SetComponent(i,j,1)
            k = k + 1
            j = j + 1

        i = i + 1

    dsa.AddArray(var)
    del var

    pass

anotherFloatArray = vtk.vtkFloatArray()
anotherFloatArray.Allocate(1,1)
anotherFloatArray.SetNumberOfComponents(3)
anotherFloatArray.SetNumberOfTuples(4)
anotherFloatArray.SetName("anotherFloatArray")

foo = ''

for attribute in "Scalars Vectors Normals TCoords".split():
    eval('dsa.SetActive'+attribute+'(\"anotherFloatArray\")')
    eval('dsa.Get'+attribute+'(\"anotherFloatArray\")')
    eval('dsa.Get'+attribute+'(foo)')

del anotherFloatArray


aFloatTensors = vtk.vtkFloatArray()
aFloatTensors.Allocate(1,1)
aFloatTensors.SetNumberOfComponents(9)
aFloatTensors.SetNumberOfTuples(4)
aFloatTensors.SetName("aFloatTensors")
i = 0
while i < aFloatTensors.GetNumberOfTuples():
    j = 0
    while j < aFloatTensors.GetNumberOfComponents():
        aFloatTensors.SetComponent(i,j,1)
        k = k + 1
        j = j + 1

    i = i + 1

dsa.AddArray(aFloatTensors)
del aFloatTensors
dsa.SetActiveTensors("aFloatTensors")
dsa.GetTensors("aFloatTensors")
dsa.GetTensors(foo)

dsa.RemoveArray("anotherFloatArray")

dsa2 = vtk.vtkDataSetAttributes()
dsa2.CopyAllocate(dsa,4,4)
dsa2.CopyData(dsa,0,0)
del dsa2

dsa3 = vtk.vtkDataSetAttributes()
dsa3.InterpolateAllocate(dsa,4,4)
dsa3.InterpolateEdge(dsa,0,0,1,0.5)

dsa4 = vtk.vtkDataSetAttributes()
dsa4.InterpolateAllocate(dsa,4,4)
dsa4.InterpolateTime(dsa,dsa3,0,0.5)
del dsa4

del dsa3
del dsa
