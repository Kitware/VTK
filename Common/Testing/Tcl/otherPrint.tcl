for {set i  0} {$i < [expr $argc - 1]} {incr i} {
   if {[lindex $argv $i] == "-A"} {
      set auto_path "$auto_path [lindex $argv [expr $i +1]]"
   }
}

package require vtk

vtkDebugLeaks rtDebugLeaks
rtDebugLeaks PromptUserOff
rtDebugLeaks Delete

vtkObject a
a GlobalWarningDisplayOff
a Delete

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
        if {$a == "vtkOutputWindow"} {
           continue
        }
	catch {
           $a b
           b Print
           if {[b IsA $a] == 0} {puts stderr "$a failed IsA test!!!"}
           if {[b IsA "vtkObject"] == 0} {puts stdout "$a is not a sub-class of vtkObject"}
           b GetClassName
           b Delete
        }
	catch {
           $a b 
           $a c 
           set d [b SafeDownCast c]
           b Delete
           c Delete
        }
	catch {
           b Delete
           c Delete
        }
    }
}

# All tests should end with the following...

rtOtherTest stdout

exit

