catch {load vtktcl}

set rtSelector "sed -e s/0x0/0/ | sed -e s/-0/0/ | grep -v -i thread | grep -v StartTime: | grep -v 0x | grep -v Modified "
set rtComparator "diff -b"

proc rtOtherTest { fileid } {
    puts $fileid "Array test started"

    foreach array "Bit Char Double Float Int Long Short UnsignedChar UnsignedInt UnsignedLong UnsignedShort" {
	puts $fileid "$array array"
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
	puts $fileid "Actual memory size is: [b${array}Array GetActualMemorySize]"
	a${array}Array Squeeze
	a${array}Array Initialize
    }
    puts $fileid "Array test ended"
}

# All tests should end with the following...

if {![info exists rtOutId]} {
    rtOtherTest stdout
    exit
}

wm withdraw .
