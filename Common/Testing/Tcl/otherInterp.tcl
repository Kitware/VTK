for {set i  0} {$i < [expr $argc - 1]} {incr i} {
   if {[lindex $argv $i] == "-A"} {
      set auto_path "$auto_path [lindex $argv [expr $i +1]]"
   }
}
package require vtk

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
   vtkCommand DebugOff
   vtkCommand DeleteAllObjects
   exit
}

wm withdraw .
