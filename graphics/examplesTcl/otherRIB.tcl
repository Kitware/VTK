# All tests need to:
# Define 2 variables and 1 proc
#     var 1: rtSelector - a shell command line to select the output to be compared
#     var 2: rtComparator - a shell command to compare the selected output with the baseline
#     proc 1: rtOtherTest (fileid)
# A few statements at the end to run outside the regression testing framework
#

catch {load vtktcl}

set rtSelector "sed -e s/-0\.00000/0\.00000/g | grep -v cells_"
set rtComparator "diff"

proc rtOtherTest { fileid } {
#actual test is blank, all the work is done with the selector
    set fid [open cells.rib "r"]
    puts $fileid [read $fid]
    close $fid
}

# All tests should end with the following...

if {![info exists rtOutId]} {
    rtOtherTest stdout
    exit
}

wm withdraw .
