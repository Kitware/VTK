for {set i  0} {$i < [expr $argc - 1]} {incr i} {
   if {[lindex $argv $i] == "-A"} {
      set auto_path "$auto_path [lindex $argv [expr $i +1]]"
   }
}

package require vtk

vtkAmoebaMinimizer m

proc func {} {
    set x  [m GetParameterValue "x"]
    set y  [m GetParameterValue "y"]
    set z  [m GetParameterValue "z"]
    
    set r  [expr ($x-5.0)*($x-5.0) + ($y+2.0)*($y+2.0) + ($z)*($z)]
    
    m SetResult $r
}

m SetFunction func
m SetParameterBracket "x" -2 2
m SetParameterBracket "y" -2 2
m SetParameterBracket "z" -2 2
m Minimize

puts "should find x=5, y=-2, z=0"

puts -nonewline "should be 0 if the simplex converged -> "
puts [m Iterate]

puts -nonewline "x = "
puts [m GetParameterValue "x"]
puts -nonewline "y = "
puts [m GetParameterValue "y"]
puts -nonewline "z = "
puts [m GetParameterValue "z"]

puts -nonewline "result = "
puts [m GetResult]

puts -nonewline "iterations = "
puts [m GetIterations]

exit