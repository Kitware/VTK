source frog.tcl

set NAME heart
set TISSUE 6
set START_SLICE 49
set END_SLICE 93
set ZMAX [expr $END_SLICE - $START_SLICE]
set VOI "217 299 186 266 0 $ZMAX"

source segmented8.tcl
