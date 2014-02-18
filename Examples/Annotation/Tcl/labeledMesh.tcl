# This example demonstrates the use of vtkLabeledDataMapper.  This class
# is used for displaying numerical data from an underlying data set.  In
# the case of this example, the underlying data are the point and cell
# ids.

#
# First we include the VTK Tcl packages which will make available
# all of the vtk commands to Tcl
#
package require vtk
package require vtkinteraction

# Create a selection window.  We will display the point and cell ids that
# lie within this window.
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

# Create a sphere and its associated mapper and actor.
vtkSphereSource sphere
vtkPolyDataMapper   sphereMapper
    sphereMapper SetInputConnection [sphere GetOutputPort]
    sphereMapper GlobalImmediateModeRenderingOn
vtkActor sphereActor
    sphereActor SetMapper sphereMapper

# Generate data arrays containing point and cell ids
vtkIdFilter ids
    ids SetInputConnection [sphere GetOutputPort]
    ids PointIdsOn
    ids CellIdsOn
    ids FieldDataOn

# Create the renderer here because vtkSelectVisiblePoints needs it.
vtkRenderer ren1

# Create labels for points
vtkSelectVisiblePoints visPts
    visPts SetInputConnection [ids GetOutputPort]
    visPts SetRenderer ren1
    visPts SelectionWindowOn
    visPts SetSelection $xmin [expr $xmin + $xLength] \
	    $ymin [expr $ymin + $yLength]

# Create the mapper to display the point ids.  Specify the
# format to use for the labels.  Also create the associated actor.
vtkLabeledDataMapper ldm
    ldm SetInputConnection [visPts GetOutputPort]
#    ldm SetLabelFormat "%g"
    ldm SetLabelModeToLabelFieldData
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
# Create the mapper to display the cell ids.  Specify the
# format to use for the labels.  Also create the associated actor.
vtkLabeledDataMapper cellMapper
    cellMapper SetInputConnection [visCells GetOutputPort]
#    cellMapper SetLabelFormat "%g"
    cellMapper SetLabelModeToLabelFieldData
    [cellMapper GetLabelTextProperty] SetColor 0 1 0
vtkActor2D cellLabels
    cellLabels SetMapper cellMapper

# Create the RenderWindow and RenderWindowInteractor
#
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer; set the background and size;
# render
ren1 AddActor sphereActor
ren1 AddActor2D rectActor
ren1 AddActor2D pointLabels
ren1 AddActor2D cellLabels

ren1 SetBackground 1 1 1
renWin SetSize 500 500
renWin Render

# Set the user method (bound to key 'u')
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# Withdraw the default tk window.
wm withdraw .

# Create a tcl procedure to move the selection window across the data set.
proc MoveWindow {} {
    for {set y 100} {$y < 300} {incr y 25} {
	for {set x 100} {$x < 300} {incr x 25} {
	    PlaceWindow $x $y
	}
    }
}

# Create a tcl procedure to draw the selection window at each location it
# is moved to.
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
    # Call Modified because InsertPoints does not modify vtkPoints
    # (for performance reasons).
    pts Modified;

    renWin Render
}

# Move the selection window across the data set.
MoveWindow
# Put the selection window in the center of the render window.
# This works because the xmin = ymin = 200, xLength = yLength = 100, and
# the render window size is 500 x 500.
PlaceWindow $xmin $ymin
iren Start