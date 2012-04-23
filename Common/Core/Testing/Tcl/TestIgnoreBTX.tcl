for {set i  0} {$i < [expr $argc - 1]} {incr i} {
   if {[lindex $argv $i] == "-A"} {
      set auto_path "$auto_path [lindex $argv [expr $i +1]]"
   }
}

package require vtk

# A simple test to see if BTX'd methods are available
proc rtOtherTest { fileid } {
  vtkStringArray stringArray
  vtkInformation information

  if {[catch {stringArray CopyInformation information 0}] == 0} {
    puts "VTK_IGNORE_BTX=ON test succeeded"
  } else {
    puts stderr "VTK_IGNORE_BTX=ON test failed"
    exit 1
  }
}

# All tests should end with the following...

rtOtherTest stdout

exit
