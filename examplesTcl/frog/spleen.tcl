source frog.tcl

set NAME spleen
set TISSUE 14
set START_SLICE 45
set END_SLICE 68
set ZMAX [expr $END_SLICE - $START_SLICE]
set VOI "166 219 195 231 0 $ZMAX"

source segmented8.tcl
