package require vtk
package require vtkinteraction

# prevent the tk window from showing up then start the event loop
wm withdraw .

vtkRenderWindow renWin

# create a rendering window and renderer
vtkRenderer ren1
    renWin AddRenderer ren1
    renWin SetSize 400 400

vtkSpherePuzzle puzzle
vtkPolyDataMapper mapper
   mapper SetInput [puzzle GetOutput]
vtkActor actor
    actor SetMapper mapper

vtkSpherePuzzleArrows arrows
vtkPolyDataMapper mapper2
   mapper2 SetInput [arrows GetOutput]
vtkActor actor2
    actor2 SetMapper mapper2




# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor
ren1 AddActor actor2

ren1 SetBackground 0.1 0.2 0.4


set LastVal -1
proc MotionCallback {x y} {
    global LastVal

    # Compute display point from Tk display point.
    set WindowY 400
    set y [expr $WindowY - $y]
    set z [ren1 GetZ $x $y]

    ren1 SetDisplayPoint $x $y $z
    ren1 DisplayToWorld
    set pt [ren1 GetWorldPoint]

    #tk_messageBox -message "$pt"
    set x [lindex $pt 0]
    set y [lindex $pt 1]
    set z [lindex $pt 2]

    set val [puzzle SetPoint $x $y $z]
    if {$val != $LastVal} {
	renWin Render
	set LastVal $val
    }
}


proc ButtonCallback {x y} {

    # Compute display point from Tk display point.
    set WindowY 400
    set y [expr $WindowY - $y]
    set z [ren1 GetZ $x $y]

    ren1 SetDisplayPoint $x $y $z
    ren1 DisplayToWorld
    set pt [ren1 GetWorldPoint]

    #tk_messageBox -message "$pt"
    set x [lindex $pt 0]
    set y [lindex $pt 1]
    set z [lindex $pt 2]

    # Had to move away from mose events (sgi RT problems)
    for { set i 0} {$i <= 100} {set i [expr $i + 5]} {
        puzzle SetPoint $x $y $z
        puzzle MovePoint $i
        renWin Render
    }


}

renWin Render

set cam [ren1 GetActiveCamera]
$cam Elevation -40
update




puzzle MoveHorizontal 0 100 0
puzzle MoveHorizontal 1 100 1
puzzle MoveHorizontal 2 100 0
puzzle MoveVertical 2 100 0
puzzle MoveVertical 1 100 0


renWin Render
