# Line Stipple Pattern demo (S. Barré, extended from squadViewer)

catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl
source $VTK_TCL/TkInteractor.tcl

set root [toplevel .top]
wm title .top "Line stipple demo"
wm protocol .top WM_DELETE_WINDOW exit

# create render window

vtkRenderWindow renWin
renWin SetPointSmoothing 0
renWin SetLineSmoothing 0
renWin SetPolygonSmoothing 0

set ren [vtkTkRenderWidget $root.ren \
        -width 400 -height 400 \
        -rw renWin]
BindTkRenderWidget $ren

set params [frame $root.params]

# create squad parameter sliders

set squadf [frame $params.squadf]

set opa [scale $squadf.opa \
        -from 0 -to 1.0 -res 0.05 \
        -orient horizontal \
	-width 10 \
        -label "Opacity:" \
        -command setOpacity]

set prs [scale $squadf.prs \
        -from 0 -to 3.5 -res 0.1 \
        -orient horizontal \
	-width 10 \
        -label "Phi roundness:" \
        -command setPhi]

set trs [scale $squadf.trs \
        -from 0 -to 3.5 -res 0.1 \
        -orient horizontal \
	-width 10 \
        -label "Theta roundness:" \
        -command setTheta]

set thicks [scale $squadf.thicks \
        -from 0.01 -to 1 -res 0.01 \
        -orient horizontal\
	-width 10 \
        -label "Thickness:" \
        -command setThickness]

set torbut [checkbutton $squadf.torbut \
        -text "Toroid" \
        -variable toroid \
        -command setToroid]

pack $opa $prs $trs $thicks $torbut -side top -anchor nw -fill both

# create line stipple parameter

set linef [frame $params.linef -relief raised]

set repeatf [scale $linef.repeatf \
        -from 1 -to 20 -res 1 \
        -orient horizontal \
        -width 10 \
        -label "Line Stipple Repeat Factor:" \
        -command setRepeatFactor]

set patternvalue [label $linef.patternvalue \
        -text "Line Stipple Pattern:"]

set pattern [frame $linef.pat]
for {set i 0} {$i < 16} {incr i} {
    set but [checkbutton $pattern.bit_$i \
            -variable patbits($i) \
            -indicatoron 0 \
            -selectcolor #000000 \
            -background #FFFFFF \
            -command setPatternFromPatternBits]
    grid $but -row 0 -column [expr 15 - $i] -sticky news
}

set presetsl [label $linef.presetsl \
        -text "Presets:"]

pack $repeatf -side top -anchor nw  -fill both
pack $patternvalue $pattern $presetsl -side top -anchor nw

foreach value {0 65535 21845 4369 13107} {
    set but [button $linef.preset_$value \
            -text [format "%X" $value] \
        -command "setPatternBitsFromValue $value ; setPatternFromPatternBits"]
    pack $but -side left -anchor nw -padx 2 -pady 2
}

pack $squadf -side left -anchor nw -fill both
pack $linef -side right -anchor ne -fill both

pack $ren $params -side top -fill both -expand yes -fill both

# create pipeline

set renWin1 [$ren GetRenderWindow]

vtkSuperquadricSource squad
    squad SetPhiResolution 20
    squad SetThetaResolution 25
    squad SetScale 1 1 1

vtkPolyDataMapper mapper
    mapper SetInput [squad GetOutput]

vtkActor actor
    actor SetMapper mapper

vtkTriangleFilter triangle
    triangle SetInput [squad GetOutput]
 
vtkFeatureEdges edges
    edges SetInput [triangle GetOutput]
    edges BoundaryEdgesOn
    edges ColoringOff
    edges ManifoldEdgesOff

vtkPolyDataMapper mapperw
    mapperw SetInput [edges GetOutput]
    mapperw SetResolveCoincidentTopologyToPolygonOffset

vtkActor actorw
    actorw SetMapper mapperw
    eval [actorw GetProperty] SetColor 0 0 0
    eval [actorw GetProperty] SetLineStipplePattern 4369

proc setOpacity {opacity} {
    eval [actor GetProperty] SetOpacity $opacity
    global renWin1
    $renWin1 Render
}

proc setPhi {phi} {
    squad SetPhiRoundness $phi
    global renWin1
    $renWin1 Render
}

proc setTheta {theta} {
    squad SetThetaRoundness $theta
    global renWin1
    $renWin1 Render
}

proc setThickness {thickness} {
    squad SetThickness $thickness
    global renWin1
    $renWin1 Render
}

proc setToroid {} {
    global toroid thicks
    squad SetToroidal $toroid
    if {$toroid} {
	$thicks config -state normal -fg black
    } else {
	$thicks config -state disabled -fg gray
    }
    global renWin1
    $renWin1 Render
}

proc setRepeatFactor {factor} {
    eval [actorw GetProperty] SetLineStippleRepeatFactor $factor
    global renWin1
    $renWin1 Render
}

proc setPatternBitsFromValue {pat} {
    global patbits
    set power 1
    for {set i 0} {$i < 16} {incr i} {
        if {[expr $pat & $power]} {
            set patbits($i) 1
        } else {
            set patbits($i) 0
        }
        set power [expr $power * 2]
    }
}

proc setPatternFromPatternBits {} {
    global patbits patternvalue
    set res 0
    set power 1
    for {set i 0} {$i < 16} {incr i} {
        if {$patbits($i)} {
            incr res $power
        }
        set power [expr $power * 2]
    }
    $patternvalue configure \
            -text [format "Line Stipple Pattern %u <=> %X :" $res $res]
    eval [actorw GetProperty] SetLineStipplePattern $res
    global renWin1
    $renWin1 Render
}

$opa set 0.3
$prs set 1.0
$trs set 0.7
$thicks set 0.3
$repeatf set 1
$torbut select ; setToroid

setPatternBitsFromValue [eval [actorw GetProperty] GetLineStipplePattern]
setPatternFromPatternBits

# Create renderer stuff

vtkRenderer ren1
ren1 SetAmbient 1 1 1
$renWin1 AddRenderer ren1

ren1 AddActor actor
ren1 AddActor actorw
ren1 SetBackground 0.6902 0.7686 0.8706
[ren1 GetActiveCamera] Zoom 1.5
[ren1 GetActiveCamera] Elevation 40
[ren1 GetActiveCamera] Azimuth -20

# prevent the tk window from showing up then start the event loop

wm withdraw .
update
