for {set i  0} {$i < [expr $argc - 1]} {incr i} {
   if {[lindex $argv $i] == "-A"} {
      set auto_path "$auto_path [lindex $argv [expr $i +1]]"
   }
}

package require vtk

vtkDataSetAttributes dsa

foreach array "Bit Char Double Float Int Long Short UnsignedChar UnsignedInt UnsignedLong UnsignedShort" {

    vtk${array}Array a${array}Array
    a${array}Array Allocate 1 1
    a${array}Array SetNumberOfComponents 3
    a${array}Array SetNumberOfTuples 4
    a${array}Array SetName a${array}Array

    # SetComponent
    set k 0
    for {set i 0} {$i < [a${array}Array GetNumberOfTuples]} {incr i} {
	for {set j 0} {$j < [a${array}Array GetNumberOfComponents]} {incr j} {
	    a${array}Array SetComponent $i $j 1
	    incr k
	}
    }

    dsa AddArray  a${array}Array
    a${array}Array Delete
}

foreach attribute "Scalars Vectors Normals TCoords" {
    dsa SetActive${attribute} aFloatArray
    dsa Get${attribute} aFloatArray
    dsa Get${attribute} foo
}

vtkFloatArray aFloatTensors
aFloatTensors Allocate 1 1
aFloatTensors SetNumberOfComponents 9
aFloatTensors SetNumberOfTuples 4
aFloatTensors SetName aFloatTensors
for {set i 0} {$i < [aFloatTensors GetNumberOfTuples]} {incr i} {
   for {set j 0} {$j < [aFloatTensors GetNumberOfComponents]} {incr j} {
      aFloatTensors SetComponent $i $j 1
      incr k
   }
}
dsa AddArray  aFloatTensors
aFloatTensors Delete
dsa SetActiveTensors aFloatTensors
dsa GetTensors aFloatTensors
dsa GetTensors foo

dsa RemoveArray aFloatArray

vtkDataSetAttributes dsa2
dsa2 CopyAllocate dsa 4 4
dsa2 CopyData dsa 0 0
dsa2 Delete

vtkDataSetAttributes dsa3
dsa3 InterpolateAllocate dsa 4 4
dsa3 InterpolateEdge dsa 0 0 1 0.5

vtkDataSetAttributes dsa4
dsa4 InterpolateAllocate dsa 4 4
dsa4 InterpolateTime dsa dsa3 0 0.5
dsa4 Delete

dsa3 Delete


dsa Delete
exit
