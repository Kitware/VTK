source frog.tcl

set NAME eye_retna
set TISSUE 4
set START_SLICE 1
set END_SLICE 41
set ZMAX [expr $END_SLICE - $START_SLICE]
set VOI "382 438 180 285 0 $ZMAX"

source segmented8.tcl
