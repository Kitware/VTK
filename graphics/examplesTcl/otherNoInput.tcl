# All tests need to:
# Define 2 variables and 1 proc
#     var 1: rtSelector - a shell command line to select the output to be compared
#     var 2: rtComparator - a shell command to compare the selected output with the baseline
#     proc 1: rtOtherTest (fileid)
# A few statements at the end to run outside the regression testing framework
#

catch {load vtktcl}

set rtSelector "cat vtkMessageLog.log | sed s/\(.\*\)// | grep -v ERROR:"
set rtComparator "diff -b"

proc rtOtherTest { fileid } {
#actual test
    vtkFileOutputWindow win
    win FlushOn
    win SetInstance win
    puts $fileid "No Input test started"

    set all [lsort [info command vtk*]]
    foreach a $all {
	puts $fileid "$a --------------"
	if {$a == "vtkIndent"} {
	    continue
	}
	if {$a == "vtkTimeStamp"} {
	    continue
	}
	if {$a == "vtkFileOutputWindow"} {
	    continue
	}
	if {$a == "vtkOutputWindow"} {
	    continue
	}
	if {$a == "vtkWin32OutputWindow"} {
	    continue
	}
	if {$a == "vtkSynchronizedTemplates3D"} {
	    continue
	}
	win DisplayText "$a-----------"
	catch {$a b; b Update;}
	catch {b Delete}
    }
}

# All tests should end with the following...

if {![info exists rtOutId]} {
    rtOtherTest stdout
    exit
}

wm withdraw .
