proc ReadCPUTimeTable { } {
    global CPUTimeTable
    global env

    if { [catch {set VTK_HISTORY_PATH $env(VTK_HISTORY_PATH)}] != 0} return

    source CPUTimeTable.tcl
    return
}

proc ComputeLimits { theList } {
    global env

    if { [catch {set VTK_HISTORY_PATH $env(VTK_HISTORY_PATH)}] != 0} return

    ## Compute the mean - use the most recent 20 samples
    set sum 0
    set validCount 0
    foreach n $theList {
	if { $n != -1 } {
	    set sum [expr $sum + $n]
	    incr validCount
	}
	if { $validCount > 20 } break
    }

    if { $validCount > 0 } {
	set mean [expr $sum / $validCount]
    } else {
	return [list 0 0]
    }

    ## Compute the standard deviation - use the most recent 20 samples
    set stddev 0
    set validCount 0
    foreach n $theList {
	if { $n != -1 } {
	    set stddev [expr $stddev + (($n - $mean) * ($n - $mean))]
	    incr validCount
	}
	if { $validCount > 20 } break
    }
    if { $validCount > 1 && $stddev > 0 } {
        set stddev [expr sqrt( $stddev / ($validCount-1) )]
    } else {
        set stddev 0
    }

    ## take the larger of the standard deviation or 0.01 times the mean
    if { [expr 0.01 * $mean] > $stddev } {
        set stddev [expr 0.01 * $mean]
    }

    ## make sure this is at least 0.01
    if { $stddev < 0.01 } {
	set stddev 0.01
    }

    ## set the limit to be 3 standard deviations around the mean - this is
    ## guaranteed to get at least 3% above and 3% below and allow at least
    ## a range of 0.06 seconds.
    set lowLimit  [expr $mean - 3*$stddev]
    set highLimit [expr $mean + 3*$stddev]

    return [list $lowLimit $highLimit]
}


proc CheckTime { theTest {currentTime -1}} {
    global CPUTimeTable
    global env

    if { [catch {set VTK_HISTORY_PATH $env(VTK_HISTORY_PATH)}] != 0} return

    set validCount 0
    set totalCount 0
    set daysAgo 1

    set theTest [file rootname $theTest]

    set timelist ""
    catch { set timelist $CPUTimeTable($theTest) }
    
    if { $timelist == "" } { return "Warning: New Test" }

    if { $currentTime == -1 } {
	set currentTime [lindex $timelist 0]
	set timelist [lrange $timelist 1 end]
    }

    if { $timelist == "" } { return "Warning: New Test" }

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
    set total_count 0
    set limit [llength $timelist]
    while { $retCode == "" && $count < 5 && $total_count < $limit} {
	set testTime [lindex $timelist 0]
        if { $testTime != -1 } {

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
        incr total_count
    }

    return $retCode
}

proc GeneratePlotFiles { theTest currentTime } {
    global CPUTimeTable
    global env
    global VTK_RESULTS_PATH

    if { [catch {set VTK_HISTORY_PATH $env(VTK_HISTORY_PATH)}] != 0} return

    set theTest [file rootname $theTest]

    set timelist ""
    catch { set timelist $CPUTimeTable($theTest) }
    
    if { $timelist == "" } { return }

    ## remove ending -1's
    set i [expr [llength $timelist] - 1]
    set done 0
    while { $i >= 0 && $done == 0 } {
	if { [lindex $timelist $i] == "-1" } {
	    set timelist [lrange $timelist 0 [expr $i - 1]]
	} else {
	    set done 1
	}
    }

    ## Get at most 10 days for the first graph
    set length [llength $timelist]
    if { $length > 10 } { set length 10 }

    set fd [open "${VTK_RESULTS_PATH}$theTest.tcl.10day.dat" w]
    if { $fd < 0 } { return }

    set limits [ComputeLimits $timelist] 
    set low  [lindex $limits 0]
    set high [lindex $limits 1]

    puts $fd "BAR_GRAPH"
    puts $fd "375 250"
    puts $fd "CPU Time History - $length days"
    puts $fd "Days Ago"
    puts $fd "CPU Seconds"
    puts $fd "1 $low $high"
    puts $fd "$length"
    for { set i $length } { $i > 0 } { incr i -1 } {
	puts $fd "$i"
    }
    puts $fd "END"
    puts $fd "1"
    puts $fd "CPU Time"
    puts $fd "$length"
    for { set i [expr $length - 2] } { $i >= 0 } { incr i -1 } {
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

    ## get at most 90 days for the second plot
    set length [llength $timelist]
    if { $length > 90 } { set length 90 }

    set fd [open "${VTK_RESULTS_PATH}$theTest.tcl.90day.dat" w]
    if { $fd < 0 } { return }

    set limits [ComputeLimits $timelist] 
    set low  [lindex $limits 0]
    set high [lindex $limits 1]

    puts $fd "BAR_GRAPH"
    puts $fd "375 250"
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
	    puts $fd "-123456"
	} else {
	    puts $fd "$v"
	}
    }
    puts $fd "$currentTime"
    puts $fd "END"

    close $fd
}
