catch {load vtktcl}
#
# This is a regression test script for VTK.
#

#
# if VTK_RESULTS_PATH is defined, then use if to qualify the error 
# and test texts
#
if { [catch {set VTK_RESULTS_PATH $env(VTK_RESULTS_PATH)/}] != 0} { set VTK_RESULTS_PATH "" }

# on windows shut off global warnings because they hold up the tests
if {$tcl_platform(os) == "Windows NT"} {
    vtkObject rtTempObject;
    rtTempObject GlobalWarningDisplayOff;
}

#
# if VTK_PLATFORM is defined, then use if to get another valid text
#
if { [catch {set VTK_PLATFORM ".$env(VTK_PLATFORM)"}] != 0} { set VTK_PLATFORM "" }

# determine where the "valid" are kept
if { [catch {set validPath $env(VTK_OTHER_VALID_PATH) }] != 0} {
    set validPath valid
} else {
    # are we in graphics or imaging or ?
    set fullPathList [file split [file dirname [pwd]]]
    set kitName [lindex $fullPathList [expr [llength $fullPathList] - 1 ]]
    
    # append the kitname onto the validPath
    set validPath "$validPath/$kitName"
}

# set up the log file descriptor
if { [catch {set logFileName $env(VTK_OTHER_REGRESSION_LOG) }] != 0} {
    set logFile stdout
} else {
    set logFile [open $logFileName "a+"]
}

# first find all the examples. they can be defined on command line or in
# current directory
if { $argv != ""} {
    set files $argv
} else {
    set files [lsort [glob {other*}]]
}

# Determine which files are executable
set allFiles $files
foreach afile $allFiles {
  if { [file executable $afile] != 1 } {
    set pos [lsearch $files $afile]
    set files [lreplace $files $pos $pos ]
  }
}

# remove files that are not suitable for regression tests or simply don't 
# work right now
set noTest {}

for {set i 0} {$i < [llength $noTest]} {incr i} {
    if {[set pos [lsearch $files [lindex $noTest $i]]] != -1} {
      set files [lreplace $files $pos $pos ]
    }
}

proc decipadString { str before total } {
    set x [string first "." $str]
    if { $x == -1 } { 
	set str "${str}.0"
    }

    set x [string first "." $str]
    while { $x >= 0 && $x < $before } {
	set str " $str"
	set x [string first "." $str]
    }

    if { [string length $str] >= $total } {
        return [string range $str 0 [expr $total - 1]]
    }

    while { [string length $str] < $total } {
        set str "${str}0"
    }
    return $str
}

# Convenience script to pad a string out to a given length
proc padString { str amount } {
    while { [string length $str] < $amount } {
        set str " $str"
    }
    return $str
}


# now do the tests
foreach afile $files {
    #
    # only tcl scripts with valid/ texts are tested
    set validOther $validPath/${afile}.cxx${VTK_PLATFORM}.rtr
    
    #
    # first see if vtk has been built --with-patented
    if { [info command vtkMarchingCubes] == "" } {
	set validOther $validPath/$afile.cxx${VTK_PLATFORM}.withoutpatented.rtr
	if {[catch {set channel [open ${validOther}]}] != 0 } {
            set validOther $validPath/$afile.cxx.withoutpatented.rtr
            if {[catch {set channel [open ${validOther}]}] != 0 } {
                set validOther $validPath/$afile.cxx${VTK_PLATFORM}.rtr
            } else {
                close $channel
            }
        } else {
            close $channel
        }
    }

    #
    # now see if there is an alternate text for this architecture
    if {[catch {set channel [open ${validOther}]}] != 0 } {
        set validOther $validPath/$afile.cxx.rtr
        if {[catch {set channel [open ${validOther}]}] != 0 } {
            puts $logFile "WARNING: There is no valid other result for $afile"
            continue
        } else {
            close $channel
        }
    } else {
        close $channel
    }
    vtkMath rtExMath
    rtExMath RandomSeed 6
    
    # Start by putting the name out - right justify it and pad to 30 characters.
    # This line MUST start with a space so that name conflicts won't occur in
    # grep statements in other files
    set Name [padString $afile 29]
    puts -nonewline $logFile "\n $Name - "
    flush stdout
    
    # Create a timer so that we can get CPU time.
    # Use the tcl time command to get wall time
    # capture the output using redirection
    vtkTimerLog timer
    set startCPU [timer GetCPUTime]
    set wallTime [decipadString [expr [lindex [time {catch {puts -nonewline "[exec $afile -S ${VTK_RESULTS_PATH}$afile.cxx.test.rtr]"}} 1] 0] / 1000000.0] 4 9]
    set endCPU [timer GetCPUTime]
    set CPUTime [decipadString [expr $endCPU - $startCPU] 3 8]
    puts -nonewline $logFile "$wallTime wall, $CPUTime cpu, "
    
    catch {set filterCommand [exec $afile -f]}
    catch {set compareCommand [exec $afile -c]}
    # if no exisiting valid text, then copy the new one
    if {[catch {set channel [open ${validOther}]}] != 0 } {
        puts $logFile "\nWARNING: Creating a valid result for $afile"
        exec cp ${VTK_RESULTS_PATH}$afile.cxx.test.rtr $validOther
        vtkCommand DeleteAllObjects
        catch {destroy .top}
        continue
    }
    close $channel

    # run the event loop quickly to map any tkwidget windows
    wm withdraw .
    update

    # creating filtered file
    catch {eval exec cat ${VTK_RESULTS_PATH}$afile.cxx.test.rtr | $filterCommand >&  ${VTK_RESULTS_PATH}$afile.cxx.test.filtered.rtr}
    # creating valid filtered file
    catch {eval exec cat $validOther | $filterCommand >& ${VTK_RESULTS_PATH}$afile.cxx.filtered.rtr}
    # creating diff file
    catch {eval exec $compareCommand ${VTK_RESULTS_PATH}$afile.cxx.test.filtered.rtr ${VTK_RESULTS_PATH}$afile.cxx.filtered.rtr >& ${VTK_RESULTS_PATH}$afile.cxx.error.rtr}
    # count the number of lines in the diff result file
    set otherError [lindex [exec wc -l ${VTK_RESULTS_PATH}$afile.cxx.error.rtr] 0]
    set otherErrorString [decipadString $otherError 4 9]

    # a test has to be off by at least threshold for us to care   
    if {$otherError == 0} {
        exec /bin/rm -f ${VTK_RESULTS_PATH}$afile.cxx.test.rtr
        # never copied over exec /bin/rm -f ${VTK_RESULTS_PATH}$afile.cxx.rtr
        exec /bin/rm -f ${VTK_RESULTS_PATH}$afile.cxx.error.rtr
        exec /bin/rm -f ${VTK_RESULTS_PATH}$afile.cxx.test.filtered.rtr
        exec /bin/rm -f ${VTK_RESULTS_PATH}$afile.cxx.filtered.rtr
        set otherStatus "Passed"
    } else {
        exec /bin/cp $validOther ${VTK_RESULTS_PATH}$afile.cxx.rtr
        set otherStatus "Failed"
    }

    # Write the text error out to the log file
    puts -nonewline $logFile "$otherErrorString otherdiff, "

    # Put the final passed or failed flag out there.
    # If it failed, say why (Other, Time)
    if { $otherStatus == "Passed" } {
        puts -nonewline $logFile "Passed    "
    } else {
        puts $logFile "Failed (Other)"
    }

    vtkCommand DeleteAllObjects
    catch {destroy .top}
    catch {destroy .geo}
}

exit
