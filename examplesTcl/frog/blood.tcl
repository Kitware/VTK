source frog.tcl

set NAME blood
set TISSUE 1
set START_SLICE 14
set END_SLICE 131
set ZMAX [expr $END_SLICE - $START_SLICE]
set VOI "33 406 62 425 0 $ZMAX"

source segmented8.tcl
