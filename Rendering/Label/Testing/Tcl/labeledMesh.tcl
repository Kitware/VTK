package require vtk
package require vtkinteraction

# demonstrate use of point labeling and the selection window

# get the interactor ui

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Create a selection window
set xmin 200
set xLength 100
set xmax [expr $xmin + $xLength]
set ymin 200
set yLength 100
set ymax [expr $ymin + $yLength]

vtkPoints pts
    pts InsertPoint 0 $xmin $ymin 0
    pts InsertPoint 1 $xmax $ymin 0
    pts InsertPoint 2 $xmax $ymax 0
    pts InsertPoint 3 $xmin $ymax 0
vtkCellArray rect
    rect InsertNextCell 5
    rect InsertCellPoint 0
    rect InsertCellPoint 1
    rect InsertCellPoint 2
    rect InsertCellPoint 3
    rect InsertCellPoint 0
vtkPolyData selectRect
    selectRect SetPoints pts
    selectRect SetLines rect
vtkPolyDataMapper2D rectMapper
    rectMapper SetInputData selectRect
vtkActor2D rectActor
    rectActor SetMapper rectMapper

# Create asphere
vtkSphereSource sphere
vtkPolyDataMapper   sphereMapper
    sphereMapper SetInputConnection [sphere GetOutputPort]
    sphereMapper GlobalImmediateModeRenderingOn
vtkActor sphereActor
    sphereActor SetMapper sphereMapper

# Generate ids for labeling
vtkIdFilter ids
    ids SetInputConnection [sphere GetOutputPort]
    ids PointIdsOn
    ids CellIdsOn
    ids FieldDataOn

# Create labels for points
vtkSelectVisiblePoints visPts
    visPts SetInputConnection [ids GetOutputPort]
    visPts SetRenderer ren1
    visPts SelectionWindowOn
    visPts SetSelection $xmin [expr $xmin + $xLength] \
	    $ymin [expr $ymin + $yLength]
vtkLabeledDataMapper ldm
    ldm SetInputConnection [visPts GetOutputPort]
#    ldm SetLabelFormat "%g"
#    ldm SetLabelModeToLabelScalars
#    ldm SetLabelModeToLabelNormals
    ldm SetLabelModeToLabelFieldData
#    ldm SetLabeledComponent 0
vtkActor2D pointLabels
    pointLabels SetMapper ldm

# Create labels for cells
vtkCellCenters cc
    cc SetInputConnection [ids GetOutputPort]
vtkSelectVisiblePoints visCells
    visCells SetInputConnection [cc GetOutputPort]
    visCells SetRenderer ren1
    visCells SelectionWindowOn
    visCells SetSelection $xmin [expr $xmin + $xLength] \
	    $ymin [expr $ymin + $yLength]
vtkLabeledDataMapper cellMapper
    cellMapper SetInputConnection [visCells GetOutputPort]
#    cellMapper SetLabelFormat "%g"
#    cellMapper SetLabelModeToLabelScalars
#    cellMapper SetLabelModeToLabelNormals
    cellMapper SetLabelModeToLabelFieldData
    [cellMapper GetLabelTextProperty] SetColor 0 1 0
vtkActor2D cellLabels
    cellLabels SetMapper cellMapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor sphereActor
ren1 AddActor2D rectActor
ren1 AddActor2D pointLabels
ren1 AddActor2D cellLabels

ren1 SetBackground 1 1 1
renWin SetSize 500 500
renWin Render


# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .
proc PlaceWindow {xmin ymin} {
    global xLength yLength

    set xmax [expr $xmin + $xLength]
    set ymax [expr $ymin + $yLength]

    visPts SetSelection $xmin $xmax $ymin $ymax
    visCells SetSelection $xmin $xmax $ymin $ymax

    pts InsertPoint 0 $xmin $ymin 0
    pts InsertPoint 1 $xmax $ymin 0
    pts InsertPoint 2 $xmax $ymax 0
    pts InsertPoint 3 $xmin $ymax 0
    pts Modified;#because insertions don't modify object - performance reasons

    renWin Render
}

proc MoveWindow {} {
    for {set y 100} {$y < 300} {incr y 25} {
	for {set x 100} {$x < 300} {incr x 25} {
	    PlaceWindow $x $y
	}
    }
}



MoveWindow
PlaceWindow $xmin $ymin
