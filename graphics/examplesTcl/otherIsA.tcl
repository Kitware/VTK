# All tests need to:
# Define 2 variables and 1 proc
#     var 1: rtSelector - a shell command line to select the output to be compared
#     var 2: rtComparator - a shell command to compare the selected output with the baseline
#     proc 1: rtOtherTest (fileid)
# A few statements at the end to run outside the regression testing framework
#

catch {load vtktcl}

set rtSelector "cat"
set rtComparator "diff -b"

proc rtOtherTest { fileid } {
#actual test
    puts $fileid "IsA test started"

    set all [lsort [info command vtk*]]
    foreach a $all {
	if {$a == "vtkIndent"} {
	    continue
	}
	if {$a == "vtkOutputPort"} {
	    continue
	}
	if {$a == "vtkFloatNormals"} {
	    continue
	}
	if {$a == "vtkFloatPoints"} {
	    continue
	}
	if {$a == "vtkFloatScalars"} {
	    continue
	}
	if {$a == "vtkFloatTCoords"} {
	    continue
	}
	if {$a == "vtkFloatTensors"} {
	    continue
	}
	if {$a == "vtkFloatVectors"} {
	    continue
	}
	if {$a == "vtkTimeStamp"} {
	    continue
	}
	if {[catch {$a b; b IsA vtkObject}] != 0} {
	    puts -nonewline $fileid "$a -------- "
	    puts $fileid "No"
	    flush $fileid
	}	    
	catch {b Delete}
    }
}

# All tests should end with the following...

if {![info exists rtOutId]} {
    rtOtherTest stdout
    exit
}

wm withdraw .
