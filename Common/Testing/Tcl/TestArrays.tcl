package require vtktcl
puts "Array test started"

foreach array "Bit Char Double Float Int Long Short UnsignedChar UnsignedInt UnsignedLong UnsignedShort" {
    puts "$array array"
    vtk${array}Array a${array}Array
    a${array}Array Allocate 1 1
    a${array}Array SetNumberOfComponents 3
    a${array}Array SetNumberOfTuples 4

    # InsertComponent
    set k 0
    for {set i 0} {$i < [a${array}Array GetNumberOfTuples]} {incr i} {
	for {set j 0} {$j < [a${array}Array GetNumberOfComponents]} {incr j} {
	    a${array}Array InsertComponent $i $j 1
	    incr k
	}     
    }
    # SetComponent
    set k 0
    for {set i 0} {$i < [a${array}Array GetNumberOfTuples]} {incr i} {
	for {set j 0} {$j < [a${array}Array GetNumberOfComponents]} {incr j} {
	    a${array}Array SetComponent $i $j 1
	    incr k
	}     
    }
    
    # DeepCopy
    vtk${array}Array b${array}Array
    b${array}Array Allocate 1000 100
    # force a resize
    b${array}Array InsertComponent 2001 1 1
    b${array}Array DeepCopy a${array}Array
    
    # MakeObject
    set m${array} [b${array}Array MakeObject]

    # confirm the deep copy
    for {set i 0} {$i < [a${array}Array GetNumberOfTuples]} {incr i} {
	for {set j 0} {$j < [a${array}Array GetNumberOfComponents]} {incr j} {
	    if { [a${array}Array GetComponent $i $j] != [b${array}Array GetComponent $i $j] } {
		puts "${array}: bad component $i $j"
	    }
	    incr k
	}
    }
    b${array}Array InsertComponent 2001 1 1
    puts "Actual memory size is: [b${array}Array GetActualMemorySize]"
    b${array}Array Resize 3000
    puts "Actual memory size after Resize is: [b${array}Array GetActualMemorySize]"

    a${array}Array Squeeze
    a${array}Array Initialize

    puts "[a${array}Array Print]"
    a${array}Array Delete
    b${array}Array Delete
}
puts "Array test ended"
exit
