source frog.tcl

set NAME ileum
set TISSUE 7
set START_SLICE 25
set END_SLICE 93
set ZMAX [expr $END_SLICE - $START_SLICE]
set VOI "172 243 201 290 0 $ZMAX"

source segmented8.tcl
