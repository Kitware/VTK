# Used for setting vertex values for clipping, cutting, and contouring tests

proc case1 { scalars IN OUT } {
$scalars InsertValue 0 $IN
$scalars InsertValue 1 $OUT
$scalars InsertValue 2 $OUT
$scalars InsertValue 3 $OUT
$scalars InsertValue 4 $OUT
$scalars InsertValue 5 $OUT
$scalars InsertValue 6 $OUT
$scalars InsertValue 7 $OUT	
    if {$IN == 1} {
	caseLabel SetText "Case 1 - 00000001"
    } else {
	caseLabel SetText "Case 1c - 11111110"
    }
}

proc case2 { scalars IN OUT } {
$scalars InsertValue 0 $IN
$scalars InsertValue 1 $IN
$scalars InsertValue 2 $OUT
$scalars InsertValue 3 $OUT
$scalars InsertValue 4 $OUT
$scalars InsertValue 5 $OUT
$scalars InsertValue 6 $OUT
$scalars InsertValue 7 $OUT	
    if {$IN == 1} {
	caseLabel SetText "Case 2 - 00000011"
    } else {
	caseLabel SetText "Case 2c - 11111100"
    }
}

proc case3 { scalars IN OUT } {
$scalars InsertValue 0 $IN
$scalars InsertValue 1 $OUT
$scalars InsertValue 2 $IN
$scalars InsertValue 3 $OUT
$scalars InsertValue 4 $OUT
$scalars InsertValue 5 $OUT
$scalars InsertValue 6 $OUT
$scalars InsertValue 7 $OUT	
    if {$IN == 1} {
	caseLabel SetText "Case 3 - 00000101"
    } else {
	caseLabel SetText "Case 3c - 11111010"
    }
}

proc case4 { scalars IN OUT } {
$scalars InsertValue 0 $IN
$scalars InsertValue 1 $OUT
$scalars InsertValue 2 $OUT
$scalars InsertValue 3 $OUT
$scalars InsertValue 4 $OUT
$scalars InsertValue 5 $OUT
$scalars InsertValue 6 $IN
$scalars InsertValue 7 $OUT	
    if {$IN == 1} {
	caseLabel SetText "Case 4 - 01000001"
    } else {
	caseLabel SetText "Case 4c - 10111110"
    }
}

proc case5 { scalars IN OUT } {
$scalars InsertValue 0 $OUT
$scalars InsertValue 1 $IN
$scalars InsertValue 2 $OUT
$scalars InsertValue 3 $OUT
$scalars InsertValue 4 $IN
$scalars InsertValue 5 $IN
$scalars InsertValue 6 $OUT
$scalars InsertValue 7 $OUT	
    if {$IN == 1} {
	caseLabel SetText "Case 5 - 00110010"
    } else {
	caseLabel SetText "Case 5c - 11001101"
    }
}

proc case6 { scalars IN OUT } {
$scalars InsertValue 0 $OUT
$scalars InsertValue 1 $IN
$scalars InsertValue 2 $OUT
$scalars InsertValue 3 $IN
$scalars InsertValue 4 $IN
$scalars InsertValue 5 $OUT
$scalars InsertValue 6 $OUT
$scalars InsertValue 7 $OUT
    if {$IN == 1} {
	caseLabel SetText "Case 6 - 00011010"
    } else {
	caseLabel SetText "Case 6c - 11100101"
    }
}

proc case7 { scalars IN OUT } {
$scalars InsertValue 0 $IN
$scalars InsertValue 1 $IN
$scalars InsertValue 2 $OUT
$scalars InsertValue 3 $OUT
$scalars InsertValue 4 $OUT
$scalars InsertValue 5 $OUT
$scalars InsertValue 6 $IN
$scalars InsertValue 7 $OUT	
    if {$IN == 1} {
	caseLabel SetText "Case 7 - 01000011"
    } else {
	caseLabel SetText "Case 7c - 10111100"
    }
}

proc case8 { scalars IN OUT } {
$scalars InsertValue 0 $IN
$scalars InsertValue 1 $IN
$scalars InsertValue 2 $OUT
$scalars InsertValue 3 $OUT
$scalars InsertValue 4 $IN
$scalars InsertValue 5 $IN
$scalars InsertValue 6 $OUT
$scalars InsertValue 7 $OUT	
    if {$IN == 1} {
	caseLabel SetText "Case 8 - 00110011"
    } else {
	caseLabel SetText "Case 8c - 11001100"
    }
}

proc case9 { scalars IN OUT } {
$scalars InsertValue 0 $OUT
$scalars InsertValue 1 $IN
$scalars InsertValue 2 $IN
$scalars InsertValue 3 $IN
$scalars InsertValue 4 $OUT
$scalars InsertValue 5 $OUT
$scalars InsertValue 6 $IN
$scalars InsertValue 7 $OUT	
    if {$IN == 1} {
	caseLabel SetText "Case 9 - 01001110"
    } else {
	caseLabel SetText "Case 9c - 10110001"
    }
}
# 27 -> 1b -> 00011011
proc case9 { scalars IN OUT } {
$scalars InsertValue 0 $IN
$scalars InsertValue 1 $IN
$scalars InsertValue 2 $OUT
$scalars InsertValue 3 $IN
$scalars InsertValue 4 $IN
$scalars InsertValue 5 $OUT
$scalars InsertValue 6 $OUT
$scalars InsertValue 7 $OUT	
    if {$IN == 1} {
	caseLabel SetText "Case 9 - 00011011"
    } else {
	caseLabel SetText "Case 9c - 11101011"
    }
}

proc case10 { scalars IN OUT } {
$scalars InsertValue 0 $IN
$scalars InsertValue 1 $OUT
$scalars InsertValue 2 $OUT
$scalars InsertValue 3 $IN
$scalars InsertValue 4 $OUT
$scalars InsertValue 5 $IN
$scalars InsertValue 6 $IN
$scalars InsertValue 7 $OUT	
    if {$IN == 1} {
	caseLabel SetText "Case 10 - 01101001"
    } else {
	caseLabel SetText "Case 10c - 10010110"
    }
}

proc case11 { scalars IN OUT } {
$scalars InsertValue 0 $IN
$scalars InsertValue 1 $OUT
$scalars InsertValue 2 $OUT
$scalars InsertValue 3 $OUT
$scalars InsertValue 4 $IN
$scalars InsertValue 5 $IN
$scalars InsertValue 6 $IN
$scalars InsertValue 7 $OUT	
    if {$IN == 1} {
	caseLabel SetText "Case 11 - 01110001"
    } else {
	caseLabel SetText "Case 11c - 10001110"
    }
}

proc case12 { scalars IN OUT } {
$scalars InsertValue 0 $OUT
$scalars InsertValue 1 $IN
$scalars InsertValue 2 $OUT
$scalars InsertValue 3 $IN
$scalars InsertValue 4 $IN
$scalars InsertValue 5 $IN
$scalars InsertValue 6 $OUT
$scalars InsertValue 7 $OUT	
    if {$IN == 1} {
	caseLabel SetText "Case 12 - 00111010"
    } else {
	caseLabel SetText "Case 12c - 11000101"
    }
}

proc case13 { scalars IN OUT } {
$scalars InsertValue 0 $OUT
$scalars InsertValue 1 $IN
$scalars InsertValue 2 $OUT
$scalars InsertValue 3 $IN
$scalars InsertValue 4 $IN
$scalars InsertValue 5 $OUT
$scalars InsertValue 6 $IN
$scalars InsertValue 7 $OUT	
    if {$IN == 1} {
	caseLabel SetText "Case 13 - 01011010"
    } else {
	caseLabel SetText "Case 13c - 10100101"
    }
}

proc case14 { scalars IN OUT } {
$scalars InsertValue 0 $IN
$scalars InsertValue 1 $OUT
$scalars InsertValue 2 $IN
$scalars InsertValue 3 $IN
$scalars InsertValue 4 $OUT
$scalars InsertValue 5 $IN
$scalars InsertValue 6 $IN
$scalars InsertValue 7 $IN
    if {$IN == 1} {
	caseLabel SetText "Case 14 - 11101101"
    } else {
	caseLabel SetText "Case 14c - 00010010"
    }
}

