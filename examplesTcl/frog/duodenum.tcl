source frog.tcl

set NAME duodenum
set TISSUE 3
set START_SLICE 35
set END_SLICE 105
set ZMAX [expr $END_SLICE - $START_SLICE]
set VOI "189 248 191 284 0 $ZMAX"

source segmented8.tcl
