source frog.tcl

set NAME skeleton
set TISSUE 13
set VALUE 64.5
set START_SLICE 1
set END_SLICE 136
set VOI "23 479 8 469 $START_SLICE $END_SLICE"
set GAUSSIAN_STANDARD_DEVIATION "1.5 1.5 1"
source segmented8.tcl
