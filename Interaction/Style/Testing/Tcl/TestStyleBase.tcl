# Get random numbers

vtkMath math
math RandomSeed 1

proc randint {min max} {
    set f [math Random $min $max]
    return [expr int($f)]
}

# Main testing proc

proc test_style {style} {

    # Event state to test

    set buttons {
        Left
        Middle
        Right
    }

    set ctrls {
        0 1
    }
    
    set shifts {
        0 1
    }

    # I do not trust timers while testing (since they trigger asynchronous
    # rendering/interaction)

    set use_timers [$style GetUseTimers]
    $style UseTimersOff
    $style AutoAdjustCameraClippingRangeOn

    puts "Testing: [$style GetClassName]"

    set iren [$style GetInteractor]
    set renwin [$iren GetRenderWindow]

    $renwin Render

    # Get renwin size and center

    set win_size [$renwin GetSize]
    set win_center_x [expr [lindex $win_size 0] / 2.0]
    set win_center_y [expr [lindex $win_size 1] / 2.0]

    vtkPropPicker pick

    set radius [expr 5 * (1 + $use_timers)]

    foreach ctrl $ctrls {
        foreach shift $shifts {
            puts -nonewline " - ctrl: $ctrl shift: $shift button:" 
            foreach button $buttons {
                puts -nonewline " $button"
                flush stdout

                # First try to find a starting position where an actor
                # can be picked (not mandatory for trackball modes).
                # Search in increasingly big area, until we reach win size
                # in that case actors might not be on screen, so reset cam

                set search $radius
                while {1} {
                    set start_x [randint [expr $win_center_x - $search] \
                                         [expr $win_center_x + $search]]
                    set start_y [randint [expr $win_center_x - $search] \
                                         [expr $win_center_x + $search]]
                    if {[pick PickProp $start_x $start_y ren1]} {
                        break
                    } else {
                        if {$search > $win_center_x || 
                        $search > $win_center_y} {
                            puts "   (resetting camera)"
                            ren1 ResetCamera
                            set search $radius
                        } else {
                            incr search 5
                        }
                    }
                }

                # Start by pressing the button

                $iren SetEventInformationFlipY \
                        $start_x $start_y $ctrl $shift 0 0 0
                $iren InvokeEvent "${button}ButtonPressEvent"

                # puts " - Starting: [$iren GetEventPosition]"

                # Now move around (alternating left and right around
                # the window center in order to compensate somehow).

                set sign 1
                for {set i 0} {$i < 2 + $use_timers} {incr i} {
                    set sign [expr $sign * -1]
                    set x [randint [expr $win_center_x + $radius * 2 * $sign] \
                                   [expr $win_center_y + $radius * $sign]]
                    set y [randint [expr $win_center_y + $radius * 2 * $sign] \
                                   [expr $win_center_y + $radius * $sign]]
                    $iren SetEventInformationFlipY $x $y $ctrl $shift 0 0 0
                    # puts " - Moving:   [$iren GetEventPosition] $ctrl $shift (was [$iren GetLastEventPosition])"
                    $iren InvokeEvent "MouseMoveEvent"

                    # If this style use timers, run OnTimer multiple times
                    
                    if {$use_timers} {
                        for {set j 0} {$j < 10} {incr j} {
                            $iren InvokeEvent "TimerEvent"
                        }
                    }
                    $renwin Render
                }

                # End by releasing the button

                $iren SetEventInformationFlipY $x $y $ctrl $shift 0 0 0
                $iren InvokeEvent "${button}ButtonReleaseEvent"
            }
            puts "."
        }
    }

    $style SetUseTimers $use_timers

    $renwin Render
}
