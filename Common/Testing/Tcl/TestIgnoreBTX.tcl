for {set i  0} {$i < [expr $argc - 1]} {incr i} {
   if {[lindex $argv $i] == "-A"} {
      set auto_path "$auto_path [lindex $argv [expr $i +1]]"
   }
}

package require vtk
package require vtkcommon

# A simple test to see if BTX'd methods are available

vtkStringArray stringArray
vtkInformation information

stringArray CopyInformation information 0

exit
