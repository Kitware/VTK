source frog.tcl

set NAME kidney
set TISSUE 8
set START_SLICE 24
set END_SLICE 78
set ZMAX [expr $END_SLICE - $START_SLICE]
set VOI "116 238 193 263 0 $ZMAX"

source segmented8.tcl
