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

    ## Get the name of the test
    set theTest [file rootname $theTest]

    ## Get the list of times for this test out of the time table
    set timelist ""
    catch { set timelist $CPUTimeTable($theTest) }

    ## remove ending -1's
    set i [expr [llength $timelist] - 1]
    set done 0
    while { $i >= 0 && $done == 0 } {
	if { [lindex $timelist $i] == "-1" } {
	    set timelist [lrange $timelist 0 [expr $i - 1]]
	    incr i -1
	} else {
	    set done 1
	}
    }
    
    ## If there is nothing in the list, then this is a new test
    if { $timelist == "" } { return "Warning: New Test" }

    ## If a current time was not passed in, use the most recent time in
    ## the list as the current time, and then remove this element from the
    ## list
    if { $currentTime == -1 } {
	set currentTime [lindex $timelist 0]
	set timelist [lrange $timelist 1 end]
    }

    ## If there is nothing in the list, then this is a new test
    if { $timelist == "" } { return "Warning: New Test" }

    ## Find the limits
    set limits [ComputeLimits $timelist]
    set low  [lindex $limits 0]
    set high [lindex $limits 1]

    set retCode ""

    ## If we have less than 5 time samples, this is a recently added test.
    ## If we can't compute a low and a high, then this must be a new test
    ## (this condition should not occur?). Otherwise, test if it is
    ## consistently slower or faster, or just slower or faster for today
    if { [llength $timelist] < 5 } {
	set retCode "Warning: Recently Added Test"
    } elseif { $low == 0 && $high == 0 } {
	set retCode "Warning: New Test"
    } elseif { $currentTime < $low } {
	set tmplist [lrange $timelist 1 end]
	set limits [ComputeLimits $tmplist]
	set low  [lindex $limits 0]
	if { [lindex $timelist 0] < $low } {
	    set retCode "Warning: Consistently Faster CPU Time"
	} else {
	    set retCode "Warning: Faster CPU Time"
	}
    } elseif { $currentTime > $high } {
	set tmplist [lrange $timelist 1 end]
	set limits [ComputeLimits $tmplist]
	set high [lindex $limits 1]
	if { [lindex $timelist 0] > $high } {
	    set retCode "Warning: Consistently Slower CPU Time"
	} else {
	    set retCode "Warning: Slower CPU Time"
	}
    } 

    ## If we haven't set a return code yet, then look back for 4 days
    ## to see if anything significant has occurred. For an event to
    ## be reported as a recent increase or decrease in time, it must
    ## be sustained for 2 days (a single spike will be ignored except
    ## for the day on which it occurred).
    set count 0
    set total_count 0
    set limit [expr [llength $timelist] - 2]
    set testTime $currentTime
    while { $retCode == "" && $count < 4 && $total_count < $limit} {
	set prevTestTime $testTime
	set testTime [lindex $timelist 0]
        if { $testTime != -1 } {

            set timelist [lrange $timelist 1 end]

            set limits [ComputeLimits $timelist]
            set low  [lindex $limits 0]
            set high [lindex $limits 1]
            
            if { $low == 0 && $high == 0 } {
                set retCode "Warning: Recently Added Test"
            } elseif { $testTime < $low && $prevTestTime < $low } {
                set retCode "Warning: Recent Time Decrease"
            } elseif { $testTime > $high && $prevTestTime > $high } {
                set retCode "Warning: Recent Time Increase"
            } 	
	    incr count
        }
        incr total_count
    }

    return $retCode
}

proc GeneratePlotFiles { theTest currentTime } {
    GeneratePlotFile $theTest 10day 9 1 $currentTime
    GeneratePlotFile $theTest 90day 89 1 $currentTime
}

proc GeneratePlotFile { theTest plotname fromDay toDay { lastTime -1 } } {
    global CPUTimeTable
    global env
    global VTK_RESULTS_PATH
    global tcl_platform
    global env

    if { [catch {set VTK_HISTORY_PATH $env(VTK_HISTORY_PATH)}] != 0} return

    if { [catch {set VTK_TIME_ARCH $env(VTK_TIME_ARCH)}] != 0} {
	set VTK_TIME_ARCH ""
    }

    if { $VTK_TIME_ARCH != "" } {
	if { $VTK_TIME_ARCH == "WinNT" } {
	    set XAxisLabel "Wall Time Seconds"
	} else {
	    set XAxisLabel "CPU Seconds"
	}
    } elseif {$tcl_platform(os) == "Windows NT"} {
	set XAxisLabel "Wall Time Seconds"
    } else {
	set XAxisLabel "CPU Seconds"
    }

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
	    incr i -1
	} else {
	    set done 1
	}
    }

    ## fromDay and toDay are off by one from the actual list index
    incr fromDay -1
    incr toDay   -1

    ## Chop off the list at the toDay
    set timelist [lrange $timelist $toDay end]

    ## How many samples do we have?
    set numSamples [expr $fromDay - $toDay + 1]
    set length [llength $timelist]
    if { $length < $numSamples } { set numSamples $length }
    if { $lastTime != -1 } { incr numSamples }

    set fd [open "${VTK_RESULTS_PATH}$theTest.tcl.$plotname.dat" w]
    if { $fd < 0 } { return }

    set limits [ComputeLimits $timelist] 
    set low  [lindex $limits 0]
    set high [lindex $limits 1]

    puts $fd "BAR_GRAPH"
    puts $fd "375 250"
    puts $fd "CPU Time History - $numSamples days"
    if { $numSamples <= 15 } {
	puts $fd "Days Ago"
    } else {
	puts $fd "Weeks Ago"
    }
    puts $fd $XAxisLabel
    puts $fd "1 $low $high"
    puts $fd "$numSamples"
    if { $numSamples <= 15 } {
	for { set i $numSamples } { $i > 0 } { incr i -1 } {
	    puts $fd "$i"
	}
    } else {
	set leftover [expr $numSamples % 7]
	for { set i 0 } { $i < $leftover } { incr i } {puts $fd " "}
	for { set i [expr $numSamples/7] } { $i > 0 } { incr i -1 } {
	    puts $fd "$i"
	    for { set j 0 } { $j < 6 } { incr j } {puts $fd " "}
	}
    }
    puts $fd "END"
    puts $fd "1"
    puts $fd "CPU Time"
    puts $fd "$numSamples"
    if { $lastTime == -1 } {
	for { set i [expr $numSamples - 1] } { $i >= 0 } { incr i -1 } {
	    set v [lindex $timelist $i]
	    if { $v == -1 } {
		puts $fd "-123456"
	    } else {
		puts $fd "$v"
	    }
	}
    } else {
	for { set i [expr $numSamples - 2] } { $i >= 0 } { incr i -1 } {
	    set v [lindex $timelist $i]
	    if { $v == -1 } {
		puts $fd "-123456"
	    } else {
		puts $fd "$v"
	    }
	}
	puts $fd "$lastTime"
    }
    puts $fd "END"

    close $fd
}

