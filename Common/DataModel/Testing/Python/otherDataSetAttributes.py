#!/usr/bin/env python

i = 0
while i < expr.expr(globals(), locals(),["argc","-","1"]):
    if (lindex(argv,i) == "-A"):
        auto_path = "" + str(auto_path) + " " + str(lindex(argv,expr.expr(globals(), locals(),["i","+1"]))) + ""
        pass
    i = i + 1

dsa = vtk.vtkDataSetAttributes()
for array in "Bit Char Double Float Int Long Short UnsignedChar UnsignedInt UnsignedLong UnsignedShort".split():
    locals()[get_variable_name("vtk", array, "Array")].locals()[get_variable_name("a", array, "Array")]()
    locals()[get_variable_name("a", array, "Array")].Allocate(1,1)
    locals()[get_variable_name("a", array, "Array")].SetNumberOfComponents(3)
    locals()[get_variable_name("a", array, "Array")].SetNumberOfTuples(4)
    locals()[get_variable_name("a", array, "Array")].SetName(locals()[get_variable_name("a", array, "Array")])
    # SetComponent
    k = 0
    i = 0
    while i < locals()[get_variable_name("a", array, "Array")].GetNumberOfTuples():
        j = 0
        while j < locals()[get_variable_name("a", array, "Array")].GetNumberOfComponents():
            locals()[get_variable_name("a", array, "Array")].SetComponent(i,j,1)
            k = k + 1
            j = j + 1

        i = i + 1

    dsa.AddArray(locals()[get_variable_name("a", array, "Array")])
    del locals()[get_variable_name("a", array, "Array")]

    pass
for attribute in "Scalars Vectors Normals TCoords".split():
    dsa.locals()[get_variable_name("SetActive", attribute, "")](aFloatArray)
    dsa.locals()[get_variable_name("Get", attribute, "")](aFloatArray)
    dsa.locals()[get_variable_name("Get", attribute, "")](foo)

    pass
aFloatTensors = vtk.vtkFloatArray()
aFloatTensors.Allocate(1,1)
aFloatTensors.SetNumberOfComponents(9)
aFloatTensors.SetNumberOfTuples(4)
aFloatTensors.SetName(aFloatTensors)
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
dsa.SetActiveTensors(aFloatTensors)
dsa.GetTensors(aFloatTensors)
dsa.GetTensors(foo)
dsa.RemoveArray(aFloatArray)
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
exit
# --- end of script --
