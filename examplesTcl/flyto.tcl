#
# flyto.tcl - fly towards a point picked with the "p" key
#
# NOTE: works best with vtkLODActor
#
set flyto.numSteps 30
set flyto.renWin renWin
set flyto.iren iren
set flyto.renderer ren1
set flyto.rate 15

proc flyto {} {
    global flyto.numSteps flyto.renWin flyto.iren flyto.renderer flyto.rate

    set numSteps ${flyto.numSteps}
    set iren ${flyto.iren}
    set renWin [$iren GetRenderWindow]
    set renderer ${flyto.renderer}

    $renWin SetDesiredUpdateRate ${flyto.rate}
    set flyTo [[$iren GetPicker] GetPickPosition]
    set flyFrom [[$renderer GetActiveCamera] GetFocalPoint]
    set dx [expr [lindex $flyTo 0] - [lindex $flyFrom 0]]
    set dy [expr [lindex $flyTo 1] - [lindex $flyFrom 1]]
    set dz [expr [lindex $flyTo 2] - [lindex $flyFrom 2]]
    set distance [expr sqrt ( $dx * $dx + $dy * $dy + $dz * $dz)]
    set dx [expr $dx / $distance]
    set dy [expr $dy / $distance]
    set dz [expr $dz / $distance]
    set delta [expr $distance / $numSteps]
    for {set i 1} { $i <= $numSteps } {incr i} {
	set focalX [expr [lindex $flyFrom 0] + ($dx * $i * $delta)]
	set focalY [expr [lindex $flyFrom 1] + ($dy * $i * $delta)]
	set focalZ [expr [lindex $flyFrom 2] + ($dz * $i * $delta)]
	[$renderer GetActiveCamera] SetFocalPoint $focalX $focalY $focalZ
        set dolly [expr 30.0 / $numSteps / 100.0 + 1.0]
        [$renderer GetActiveCamera] Dolly $dolly
        $renWin Render
    }
   $renWin SetDesiredUpdateRate [$iren GetStillUpdateRate];
   $renWin Render
}
if {[info commands fastPicker] == ""} {vtkWorldPointPicker fastPicker}
${flyto.iren} SetPicker fastPicker
${flyto.iren} SetEndPickMethod  {flyto}
