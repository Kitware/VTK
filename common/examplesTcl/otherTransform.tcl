# all tests need: code to parse the args
# call to Test, and clean up code at the end
#

catch {load vtktcl}
wm withdraw .

proc Test { fileid } {
#actual test
    puts $fileid "transform test started"

    vtkTransform trans
    puts $fileid "all intances: \n[vtkCommand ListAllInstances]"
    puts $fileid "transform: "
    outputObj trans "trans" $fileid
    
    trans RotateWXYZ 20 1 1 2
    puts $fileid "RotateWXYZ 20 1 1 2"
    outputObj trans "trans" $fileid
    
    trans Translate 1 10 2
    puts $fileid "Translate 1 10 2"
    outputObj trans "trans" $fileid
    
    trans Identity
    puts $fileid "Identity"
    outputObj trans "trans" $fileid

    puts $fileid "transform test completed"

    trans Delete
}


proc outputObj {obj name fileid} {
    puts $fileid "$name : [$obj Print]" 
}


#  first process the arguments.  this is where the test result path is 
#  specified to the test, and where the test type, selector and comparator
#  are specified to the testing script.  
set outid stdout
set fileout 0
set outputProcessor(otherTransform.tcl,Selector) "grep -v vtkTransform | grep -v Modified"
set outputProcessor(otherTransform.tcl,Comparator) "diff"
set outputProcessor(otherTransform.tcl,Type) "rtr"

if {$argc == 0} {
    puts stdout "1. outputting to stdout. --h for options"
    set outid stdout
    set fileout 0
} else {
    if {[lindex $argv 0] == "--S"} {
        if { $argc >= 2 } {
            set outid [open [lindex $argv 1] "w"]
            set fileout 1
        } else {
            puts stdout "2. outputting to stdout. --h for options"
            set outid stdout
            set fileout 0
        }
    } elseif {[lindex $argv 0] == "--f"} {
        puts -nonewline stdout "$outputProcessor(otherTransform.tcl,Selector)"
        exit
    } elseif {[lindex $argv 0] == "--c"} {
        puts -nonewline stdout "$outputProcessor(otherTransform.tcl,Comparator)"
        exit
    } elseif {[lindex $argv 0] == "--e"} {
        puts -nonewline stdout "$outputProcessor(otherTransform.tcl,Type)"
        exit
    } else {
        puts stdout "optional parameters are"
        puts stdout "       --S file    path and filename"
        puts stdout "       --f         print filter command string"
        puts stdout "       --c         print comparator command string"
        puts stdout "       --e         type and extension of result file"
        exit
    }
}

Test $outid
exit


#clean up
if {$fileout == 1} {
    close $outid
}

