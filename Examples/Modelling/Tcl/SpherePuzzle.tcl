#
# First we include the VTK Tcl packages which will make available 
# all of the vtk commands to Tcl
#
package require vtk
package require vtkinteraction

#
# Prevent the tk window from showing up then start the event loop
#
wm withdraw .

#
# Create the toplevel window
#
toplevel .top
wm title .top "Sphere Puzzle"
wm protocol .top WM_DELETE_WINDOW ::vtk::cb_exit

#
# Create some frames
#
frame .top.f1 
frame .top.f2

pack .top.f1 \
        -side top -anchor n \
        -expand 1 -fill both

pack .top.f2 \
        -side bottom -anchor s \
        -expand t -fill x

#
# Create the Tk render widget, and bind the events
#
vtkRenderWindow renWin

vtkRenderer ren1
    renWin AddRenderer ren1

vtkTkRenderWidget .top.f1.rw \
        -width 400 \
        -height 400 \
        -rw renWin

::vtk::bind_tk_render_widget .top.f1.rw

pack .top.f1.rw \
        -expand t -fill both

#
# Display some infos
#
label .top.f2.l1 -text "Position cursor over the rotation plane."
label .top.f2.l2 -text "Moving pieces will be highlighted."
label .top.f2.l3 -text "Press 'm' to make a move."

button .top.f2.reset \
        -text "Reset" \
	-command {puzzle Reset; renWin Render}

button .top.f2.b1 \
        -text "Quit" \
        -command ::vtk::cb_exit

pack .top.f2.l1 .top.f2.l2 .top.f2.l3 .top.f2.reset .top.f2.b1 \
	-side top \
        -expand t -fill x

#
# Create the pipeline
#
vtkSpherePuzzle puzzle

vtkPolyDataMapper mapper
   mapper SetInputConnection [puzzle GetOutputPort]

vtkActor actor
    actor SetMapper mapper

vtkSpherePuzzleArrows arrows

vtkPolyDataMapper mapper2
   mapper2 SetInputConnection [arrows GetOutputPort]

vtkActor actor2
    actor2 SetMapper mapper2

#
# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor
ren1 AddActor actor2
ren1 SetBackground 0.1 0.2 0.4

ren1 ResetCamera
set cam [ren1 GetActiveCamera]
$cam Elevation -40
renWin Render

#
# Modify some bindings, use the interactor style 'switch'
#
set iren [renWin GetInteractor]
set istyle [vtkInteractorStyleSwitch istyleswitch]
$iren SetInteractorStyle $istyle
$istyle SetCurrentStyleToTrackballCamera

$iren AddObserver MouseMoveEvent MotionCallback
$iren AddObserver CharEvent CharCallback

#
# Highlight pieces
#
proc MotionCallback {} {

    global in_piece_rotation
    if {[info exists in_piece_rotation]} {
        return
    }

    global LastVal

    set iren [renWin GetInteractor]
    set istyle [[$iren GetInteractorStyle] GetCurrentStyle]

    # Return if the user is performing interaction

    if {[$istyle GetState]} {
        return
    }

    # Get mouse position

    set pos [$iren GetEventPosition]
    set x [lindex $pos 0]
    set y [lindex $pos 1]

    # Get world point

    ren1 SetDisplayPoint $x $y [ren1 GetZ $x $y]
    ren1 DisplayToWorld
    set pt [ren1 GetWorldPoint]

    set val [puzzle SetPoint [lindex $pt 0] [lindex $pt 1] [lindex $pt 2]]
    if {![info exists LastVal] || $val != $LastVal} {
	renWin Render
	set LastVal $val
    }
}

#
# Rotate the puzzle
#
proc CharCallback {} {

    set iren [renWin GetInteractor]

    set keycode [$iren GetKeyCode]
    if {$keycode != "m" && $keycode != "M"} {
        return
    }

    set pos [$iren GetEventPosition]
    ButtonCallback [lindex $pos 0] [lindex $pos 1]
}

proc ButtonCallback {x y} {

    global in_piece_rotation
    if {[info exists in_piece_rotation]} {
        return
    }
    set in_piece_rotation 1

    # Get world point

    ren1 SetDisplayPoint $x $y [ren1 GetZ $x $y]
    ren1 DisplayToWorld
    set pt [ren1 GetWorldPoint]

    set x [lindex $pt 0]
    set y [lindex $pt 1]
    set z [lindex $pt 2]

    for { set i 0} {$i <= 100} {set i [expr $i + 10]} {
	puzzle SetPoint $x $y $z
	puzzle MovePoint $i
	renWin Render
        update
    }

    unset in_piece_rotation
}

update

#
# Shuffle the puzzle
#
ButtonCallback 218 195
ButtonCallback 261 128
ButtonCallback 213 107
ButtonCallback 203 162
ButtonCallback 134 186

tkwait window .top
