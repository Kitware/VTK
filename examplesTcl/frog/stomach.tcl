source frog.tcl

set NAME stomach
set TISSUE 15
set START_SLICE 26
set END_SLICE 119
set ZMAX [expr $END_SLICE - $START_SLICE]
set VOI "143 365 158 297 0 $ZMAX"

source segmented8.tcl
