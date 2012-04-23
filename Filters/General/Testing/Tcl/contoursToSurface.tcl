package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

#
# Create the data
#

vtkPoints points
vtkCellArray polys

set i 0

for { set z -5 } { $z < 30 } { incr z } {
    for { set xtraX 0 } { $xtraX < 90 } { incr xtraX 30 } {
	for { set xtraY 0 } { $xtraY < 90 } { incr xtraY 30 } {

	    set x -10
	    set y -10

	    set x [expr $x + $xtraX]
	    set y [expr $y + $xtraY]

	    if { [expr $z % 12] == 0 }  { incr x 1; }
	    if { [expr $z % 12] == 1 }  { incr x 2 }
	    if { [expr $z % 12] == 2 }  { incr x 3 }
	    if { [expr $z % 12] == 3 }  { incr x 3; incr y 1 }
	    if { [expr $z % 12] == 4 }  { incr x 3; incr y 2 }
	    if { [expr $z % 12] == 5 }  { incr x 3; incr y 3 }
	    if { [expr $z % 12] == 6 }  { incr x 2; incr y 3 }
	    if { [expr $z % 12] == 7 }  { incr x 1; incr y 3 }
	    if { [expr $z % 12] == 8 }  { incr y 3 }
	    if { [expr $z % 12] == 9 }  { incr y 2 }
	    if { [expr $z % 12] == 10 } { incr y 1 }

	    if { [expr ($xtraX != 30 && $xtraY != 30) || ($xtraX == $xtraY)] } {
		polys InsertNextCell 4
		points InsertPoint $i [expr $x +  0] [expr $y +  0] $z
		polys InsertCellPoint $i
		incr i
		points InsertPoint $i [expr $x + 20] [expr $y +  0] $z
		polys InsertCellPoint $i
		incr i
		points InsertPoint $i [expr $x + 20] [expr $y + 20] $z
		polys InsertCellPoint $i
		incr i
		points InsertPoint $i [expr $x +  0] [expr $y + 20] $z
		polys InsertCellPoint $i
		incr i

		polys InsertNextCell 4
		points InsertPoint $i [expr $x +  4] [expr $y +  4] $z
		polys InsertCellPoint $i
		incr i
		points InsertPoint $i [expr $x + 16] [expr $y +  4] $z
		polys InsertCellPoint $i
		incr i
		points InsertPoint $i [expr $x + 16] [expr $y + 16] $z
		polys InsertCellPoint $i
		incr i
		points InsertPoint $i [expr $x +  4] [expr $y + 16] $z
		polys InsertCellPoint $i
		incr i
	    }

	    if { [expr $xtraX != 30 || $xtraY != 30]} {
		polys InsertNextCell 4
		points InsertPoint $i [expr $x +  8] [expr $y +  8] $z
		polys InsertCellPoint $i
		incr i
		points InsertPoint $i [expr $x + 12] [expr $y +  8] $z
		polys InsertCellPoint $i
		incr i
		points InsertPoint $i [expr $x + 12] [expr $y + 12] $z
		polys InsertCellPoint $i
		incr i
		points InsertPoint $i [expr $x +  8] [expr $y + 12] $z
		polys InsertCellPoint $i
		incr i
	    }
	}
    }
}

#
# Create a representation of the contours used as input
#

vtkPolyData contours
    contours SetPoints points
    contours SetPolys polys

vtkPolyDataMapper contourMapper
    contourMapper SetInputData contours

vtkActor contourActor
    contourActor SetMapper contourMapper
    [contourActor GetProperty] SetColor 1 0 0
    [contourActor GetProperty] SetAmbient 1
    [contourActor GetProperty] SetDiffuse 0
    [contourActor GetProperty] SetRepresentationToWireframe


ren1 AddViewProp contourActor

ren1 ResetCamera
[ren1 GetActiveCamera] Azimuth   10
[ren1 GetActiveCamera] Elevation 30
ren1 ResetCameraClippingRange

renWin SetSize 300 300
renWin Render

iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .

#
# Create the contour to surface filter
#

vtkVoxelContoursToSurfaceFilter f
f SetInputData contours
f SetMemoryLimitInBytes 100000

vtkPolyDataMapper m
m SetInputConnection [f GetOutputPort]
m ScalarVisibilityOff
m ImmediateModeRenderingOn

vtkActor a
a SetMapper m

ren1 AddViewProp a
contourActor VisibilityOff

ren1 SetBackground .1 .2 .4

renWin Render
