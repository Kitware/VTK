source frog.tcl

set NAME lung
set TISSUE 11
set START_SLICE 24
set END_SLICE 59
set ZMAX [expr $END_SLICE - $START_SLICE]
set VOI "222 324 157 291 0 $ZMAX"

source segmented8.tcl
