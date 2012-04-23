for {set i  0} {$i < [expr $argc - 1]} {incr i} {
   if {[lindex $argv $i] == "-A"} {
      set auto_path "$auto_path [lindex $argv [expr $i +1]]"
   }
}

package require vtk
package require vtkcommoncore
package require vtkiocore

# Pass and return a string by value.
vtkDelimitedTextWriter r1
set t1 [r1 GetString "hello"]
if { $t1 != "\"hello\"" } {
  puts -nonewline "return-string-by-value failed "
  puts -nonewline $t1
  puts -nonewline " != "
  puts "\"hello\""
  exit 1
}

# Pass a string by reference - can't find a good example,
# but it is coded identical to passing a string by value

# Return a string by reference.
vtkStringArray a3
set s3 "hello"
a3 InsertNextValue $s3
set t3 [a3 GetValue 0]
if { $t3 != $s3 } {
  puts -nonewline "return-string-by-reference failed "
  puts -nonewline $t3
  puts -nonewline " != "
  puts $s3
  exit 1
}

exit
