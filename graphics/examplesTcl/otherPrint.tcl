# All tests need to:
# Define 2 variables and 1 proc
#     var 1: rtSelector - a shell command line to select the output to be compared
#     var 2: rtComparator - a shell command to compare the selected output with the baseline
#     proc 1: rtOtherTest (fileid)
# A few statements at the end to run outside the regression testing framework
#

catch {load vtktcl}

set rtSelector "sed -e s/0x0/0/ | sed -e s/-0/0/ | grep -v -i thread | grep -v StartTime: | grep -v 0x | grep -v Modified | grep -v 'Compute Time:'"
set rtSelector "sed -e s/0x0/0/ | sed -e s/-0/0/ | grep -v -i thread | grep -v Time: | grep -v 0x | grep -v Modified"
set rtComparator "diff -b"

proc rtOtherTest { fileid } {
#actual test
    puts $fileid "Printf test started"

    set all [lsort [info command vtk*]]
    foreach a $all {
	puts $fileid "$a --------------"
	if {$a == "vtkIndent"} {
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
	if {$a == "vtkOutputPort"} {
	    continue
	}
	if {$a == "vtkTimeStamp"} {
	    continue
	}
	catch {$a b; puts $fileid "[b Print]"; b Delete}
    }
}

# All tests should end with the following...

if {![info exists rtOutId]} {
    rtOtherTest stdout
    exit
}

wm withdraw .
