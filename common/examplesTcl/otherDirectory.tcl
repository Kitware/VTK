# All tests need to:
# Define 2 variables and 1 proc
#     var 1: rtSelector - a shell command line to select the output to be compared
#     var 2: rtComparator - a shell command to compare the selected output with the baseline
#     proc 1: rtOtherTest (fileid)
# A few statements at the end to run outside the regression testing framework
#

catch {load vtktcl}

set rtSelector "sed -e s/0x0/0/ | sed -e s/-0/0/ | grep -v -i thread | grep -v StartTime: | grep -v 0x | grep -v Modified "
set rtComparator "diff -b"

proc rtOtherTest { fileid } {
#actual test
    puts $fileid "vtkDirectory test started"

    vtkDirectory a
    a Open "."
    set foundIt 0
    set n [a GetNumberOfFiles]
    for {set i 0} {$i < $n} {incr i} {
	if {[a GetFile $i] == "otherDirectory.tcl"} {
	    set foundIt 1
	}
    }
    if {$foundIt > 0} {
	puts $fileid "vtkDirectory OK!"
    } else {
	puts $fileid "vtkDirectory Broken!"
    }
}

# All tests should end with the following...

if {![info exists rtOutId]} {
    rtOtherTest stdout
    exit
}

wm withdraw .
