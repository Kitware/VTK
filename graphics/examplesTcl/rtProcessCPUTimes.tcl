proc ReadCPUTimeTable { } {
    global CPUTimeTable

    set OUTPUT_FILE CPUTimeTable.tcl

    source $OUTPUT_FILE
}

proc ComputeLimits { theList } {
    
    set sum 0
    set validCount 0
    foreach n $theList {
	if { $n != -1 } {
	    set sum [expr $sum + $n]
	    incr validCount
	}
	if { $validCount > 10 } break
    }

    if { $validCount > 0 } {
	set mean [expr $sum / $validCount]
    } else {
	return [list 0 0]
    }

    set stddev 0
    set validCount 0
    foreach n $theList {
	if { $n != -1 } {
	    set stddev [expr $stddev + (($n - $mean) * ($n - $mean))]
	    incr validCount
	}
	if { $validCount > 10 } break
    }

    if { $validCount > 1 && $stddev > 0 } {
        set stddev [expr sqrt( $stddev / ($validCount-1) )]
    } else {
        set stddev 0
    }

    if { [expr 0.01 * $mean] > $stddev } {
        set stddev [expr 0.01 * $mean]
    }

    if { $stddev < 0.01 } {
	set stddev 0.01
    }

    set lowLimit  [expr $mean - 3*$stddev]
    set highLimit [expr $mean + 3*$stddev]

    return [list $lowLimit $highLimit]
}


proc CheckTime { theTest currentTime } {
    global CPUTimeTable

    set validCount 0
    set totalCount 0
    set daysAgo 1

    set theTest [file rootname $theTest]

    set timelist ""
    catch { set timelist $CPUTimeTable($theTest) }
    
    if { $timelist == "" } { return 0 }

    set limits [ComputeLimits $timelist]
    set low  [lindex $limits 0]
    set high [lindex $limits 1]

    set retCode ""

    if { $low == 0 && $high == 0 } {
	set retCode "Warning: New Test"
    } elseif { $currentTime < $low } {
	set retCode "Warning: Faster CPU Time"
    } elseif { $currentTime > $high } {
	set retCode "Warning: Slower CPU Time"
    } 


    set count 0
    while { $retCode == "" && $count < 5 } {
	set testTime [lindex $timelist 0]
	set timelist [lrange $timelist 1 end]

	set limits [ComputeLimits $timelist]
	set low  [lindex $limits 0]
	set high [lindex $limits 1]

	if { $low == 0 && $high == 0 } {
	    set retCode "Warning: Recently Added Test"
	} elseif { $testTime < $low } {
	    set retCode "Warning: Recent Time Decrease"
	} elseif { $testTime > $high } {
	    set retCode "Warning: Recent Time Increase"
	} 	
	incr count
    }

    return $retCode
}

proc GeneratePlotFiles { theTest currentTime } {
    global CPUTimeTable

    set theTest [file rootname $theTest]

    set timelist ""
    catch { set timelist $CPUTimeTable($theTest) }
    
    if { $timelist == "" } { return }

    set fd [open "$theTest.tcl.10day.dat" w]
    if { $fd < 0 } { return }

    set limits [ComputeLimits $timelist] 
    set low  [lindex $limits 0]
    set high [lindex $limits 1]

    puts $fd "BAR_GRAPH"
    puts $fd "450 250"
    puts $fd "CPU Time History - 10 days"
    puts $fd "Days Ago"
    puts $fd "CPU Seconds"
    puts $fd "1 $low $high"
    puts $fd "10"
    for { set i 10 } { $i > 0 } { incr i -1 } {
	puts $fd "$i"
    }
    puts $fd "END"
    puts $fd "1"
    puts $fd "CPU Time"
    puts $fd "10"
    for { set i 8 } { $i >= 0 } { incr i -1 } {
	set v [lindex $timelist $i]
	if { $v == -1 } {
	    puts $fd "-123456"
	} else {
	    puts $fd "$v"
	}
    }
    puts $fd "$currentTime"
    puts $fd "END"

    close $fd

    set length [llength $timelist]

    set fd [open "$theTest.tcl.90day.dat" w]
    if { $fd < 0 } { return }

    set limits [ComputeLimits $timelist] 
    set low  [lindex $limits 0]
    set high [lindex $limits 1]

    puts $fd "BAR_GRAPH"
    puts $fd "450 250"
    puts $fd "CPU Time History - $length days"
    puts $fd "Weeks Ago"
    puts $fd "CPU Seconds"
    puts $fd "1 $low $high"
    puts $fd "$length"
    set leftover [expr $length % 7]
    for { set i 0 } { $i < $leftover } { incr i } {puts $fd " "}
    for { set i [expr $length/7] } { $i > 0 } { incr i -1 } {
	puts $fd "$i"
	for { set j 0 } { $j < 6 } { incr j } {puts $fd " "}
    }
    puts $fd "END"
    puts $fd "1"
    puts $fd "CPU Time"
    puts $fd "$length"
    for { set i [expr $length-2] } { $i >= 0 } { incr i -1 } {
	set v [lindex $timelist $i]
	if { $v == -1 } {
	    puts $fd "0"
	} else {
	    puts $fd "$v"
	}
    }
    puts $fd "$currentTime"
    puts $fd "END"

    close $fd
}
