source frog.tcl

set NAME nerve
set TISSUE 12
set START_SLICE 7
set END_SLICE 113
set ZMAX [expr $END_SLICE - $START_SLICE]
set VOI "79 403 63 394 0 $ZMAX"

source segmented8.tcl
