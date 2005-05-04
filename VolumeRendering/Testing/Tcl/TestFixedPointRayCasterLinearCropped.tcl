source [file join [file dirname [info script]] TestFixedPointRayCasterNearest.tcl]

for { set j 0 } { $j < 5 } { incr j } {
   for { set i 0 } { $i < 5 } { incr i } {
      volumeMapper${i}${j} SetCroppingRegionPlanes 10 20 10 20 10 20
      volumeMapper${i}${j} SetCroppingRegionFlags 253440
      volumeMapper${i}${j} CroppingOn

      volumeProperty${i}${j} SetInterpolationTypeToLinear
   }
}

renWin Render

