source frog.tcl

set NAME skeleton
set TISSUE 13
set VALUE 368.5
set START_SLICE 1
set END_SLICE 136
set ZMAX [expr $END_SLICE - $START_SLICE]
set VOI "23 479 8 473 0 $ZMAX"
set GAUSSIAN_STANDARD_DEVIATION "1.5 1.5 1"
source segmented8.tcl
