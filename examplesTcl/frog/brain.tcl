source frog.tcl

set NAME brain
set TISSUE 2
set START_SLICE 1
set END_SLICE 33
set ZMAX [expr $END_SLICE - $START_SLICE]
set VOI "349 436 211 252 0 $ZMAX"

source segmented8.tcl
