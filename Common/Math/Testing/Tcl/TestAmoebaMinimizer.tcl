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

    m SetFunctionValue $r
}

m SetFunction func
m SetParameterValue "x" 0.0
m SetParameterScale "x" 2.0
m SetParameterValue "y" 0.0
m SetParameterScale "y" 2.0
m SetParameterValue "z" 0.0
m SetParameterScale "z" 2.0
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

puts -nonewline "function value = "
puts [m GetFunctionValue]

puts -nonewline "evaluations = "
puts [m GetFunctionEvaluations]

puts -nonewline "iterations = "
puts [m GetIterations]

puts "To improve coverage and catch errors, do a PrintSelf with parameters set:"

puts [m Print]

m Delete

exit