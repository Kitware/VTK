for {set i  0} {$i < [expr $argc - 1]} {incr i} {
   if {[lindex $argv $i] == "-A"} {
      set auto_path "$auto_path [lindex $argv [expr $i +1]]"
   }
}

package require vtktcl


proc rtOtherTest { fileid } {
#actual test
    set all [lsort [info command vtk*]]
    foreach a $all {
	if {$a == "vtkIndent"} {
	    continue
	}
	if {$a == "vtkOutputPort"} {
	    continue
	}
	if {$a == "vtkTimeStamp"} {
	    continue
	}
	catch {$a b; b Print; b Delete}
    }
}

# All tests should end with the following...

if {![info exists rtOutId]} {
    rtOtherTest stdout
    exit
}

wm withdraw .
