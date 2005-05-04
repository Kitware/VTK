source [file join [file dirname [info script]] TestFixedPointRayCasterNearest.tcl]

for { set j 0 } { $j < 5 } { incr j } {
   for { set i 0 } { $i < 5 } { incr i } {
      volumeProperty${i}${j} SetInterpolationTypeToLinear
   }
}

renWin Render

