# All tests need to:
# Define 2 variables and 1 proc
#     var 1: rtSelector - a shell command line to select the output to be compared
#     var 2: rtComparator - a shell command to compare the selected output with the baseline
#     proc 1: rtOtherTest (fileid)
# A few statements at the end to run outside the regression testing framework
#

catch {load vtktcl}

set rtSelector "grep -v 0x | grep -v Modified"
set rtComparator "diff"

proc rtOtherTest { fileid } {
#actual test
    puts $fileid "transform test started"

    vtkTransform trans
    puts $fileid "transform: "
    puts $fileid "trans [trans Print]"
    
    trans RotateWXYZ 20 1 1 2
    puts $fileid "RotateWXYZ 20 1 1 2"
    puts $fileid "trans [trans Print]"
    
    trans Translate 1 10 2
    puts $fileid "Translate 1 10 2"
    puts $fileid "trans [trans Print]"
    
    trans Identity
    puts $fileid "Identity"
    puts $fileid "trans [trans Print]"

    puts $fileid "transform test completed"

    trans Delete
}

# All tests should end with the following...

if {![info exists rtOutId]} {
    rtOtherTest stdout
    exit
}

wm withdraw .
