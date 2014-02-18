# This script shows how to manually create a vtkPolyData with a
# triangle strip.

package require vtk
package require vtkinteraction

# First we'll create some points.
#
vtkPoints points
    points InsertPoint 0 0.0 0.0 0.0
    points InsertPoint 1 0.0 1.0 0.0
    points InsertPoint 2 1.0 0.0 0.0
    points InsertPoint 3 1.0 1.0 0.0
    points InsertPoint 4 2.0 0.0 0.0
    points InsertPoint 5 2.0 1.0 0.0
    points InsertPoint 6 3.0 0.0 0.0
    points InsertPoint 7 3.0 1.0 0.0

# The cell array can be thought of as a connectivity list.
# Here we specify the number of points followed by that number of
# point ids. This can be repeated as many times as there are
# primitives in the list.
#
vtkCellArray strips
    strips InsertNextCell 8;#number of points
    strips InsertCellPoint 0
    strips InsertCellPoint 1
    strips InsertCellPoint 2
    strips InsertCellPoint 3
    strips InsertCellPoint 4
    strips InsertCellPoint 5
    strips InsertCellPoint 6
    strips InsertCellPoint 7
vtkPolyData profile
    profile SetPoints points
    profile SetStrips strips

vtkPolyDataMapper map
    map SetInputData profile

vtkActor strip
    strip SetMapper map
    [strip GetProperty] SetColor 0.3800 0.7000 0.1600

# Create the usual rendering stuff.
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor strip

ren1 SetBackground 1 1 1
renWin SetSize 250 250
renWin Render

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .
iren Start


