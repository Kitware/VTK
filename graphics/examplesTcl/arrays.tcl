catch {load vtkdll}

foreach array "Bit Char Double Float Int Long Short UnsignedChar UnsignedInt UnsignedLong UnsignedShort" {
  vtk${array}Array a${array}Array
   a${array}Array SetNumberOfComponents 3
   a${array}Array SetNumberOfTuples 4

  set k 0
  for {set i 0} {$i < [a${array}Array GetNumberOfTuples]} {incr i} {
      for {set j 0} {$j < [a${array}Array GetNumberOfComponents]} {incr j} {
          a${array}Array InsertComponent $i $j 1
          incr k
      }     
  }

  vtk${array}Array b${array}Array
    b${array}Array DeepCopy a${array}Array

# confirm the deep copy
  for {set i 0} {$i < [a${array}Array GetNumberOfTuples]} {incr i} {
      for {set j 0} {$j < [a${array}Array GetNumberOfComponents]} {incr j} {
          if { [a${array}Array GetComponent $i $j] != [b${array}Array GetComponent $i $j] } {
  	    puts "${array}: bad component $i $j"
  	}
          incr k
      }
  }
}
vtkCommand DeleteAllObjects
exit

