source frog.tcl

set NAME liver
set TISSUE 10
set START_SLICE 25
set END_SLICE 126
set ZMAX [expr $END_SLICE - $START_SLICE]
set VOI "167 297 154 304 0 $ZMAX"

source segmented8.tcl
