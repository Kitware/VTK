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
    puts $fileid "vtkTclUtil test started"

    vtkCommand DebugOn
    vtkTriangle a
    vtkQuad b
    puts $fileid "a ListInstances: [a ListInstances]"
    puts $fileid "vtkTriangle ListInstances: [vtkTriangle ListInstances]"
    puts $fileid "vtkCommand ListMethods"
    puts $fileid "[vtkCommand ListMethods]"
    puts $fileid "vtkCommand ListAllInstances"
    puts $fileid "[vtkCommand ListAllInstances]"
    puts $fileid "Some error handling..."

    puts $fileid "vtkWedge"
    set rc [catch {puts $fileid "[vtkWedge]"} status]
    puts $fileid "tcl return code: $rc : $status"

    puts $fileid "vtkWedge 1a"
    set rc [catch {puts $fileid "[vtkWedge 1a]"} status]
    puts $fileid "tcl return code: $rc : $status"

    puts $fileid "vtkTransform t; vtkTransform t"
    set rc [catch {puts $fileid "[vtkTransform t; vtkTransform t]"} status]
    puts $fileid "tcl return code: $rc : $status"

    puts $fileid "vtkTransform image;"
    set rc [catch {puts $fileid "[vtkTransform image]"} status]
    puts $fileid "tcl return code: $rc : $status"

    puts $fileid "vtkTclUtil test ended"
}

# All tests should end with the following...

if {![info exists rtOutId]} {
    rtOtherTest stdout
    exit
}

wm withdraw .
