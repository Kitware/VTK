source frog.tcl

set NAME l_intestine
set TISSUE 9
set START_SLICE 56
set END_SLICE 106
set ZMAX [expr $END_SLICE - $START_SLICE]
set VOI "115 224 209 284 0 $ZMAX"

source segmented8.tcl
