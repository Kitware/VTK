catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# demonstrate use of point labeling and the selection window

# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkPoints pts1
    pts1 InsertPoint 0 100 100 0
    pts1 InsertPoint 1 250 100 0
    pts1 InsertPoint 2 250 250 0
    pts1 InsertPoint 3 100 250 0

vtkPoints pts2
    pts2 InsertPoint 0 175 175 0
    pts2 InsertPoint 1 325 175 0
    pts2 InsertPoint 2 325 325 0
    pts2 InsertPoint 3 175 325 0

vtkPoints pts3
    pts3 InsertPoint 0 250 250 0
    pts3 InsertPoint 1 400 250 0
    pts3 InsertPoint 2 400 400 0
    pts3 InsertPoint 3 250 400 0

vtkCellArray rect
    rect InsertNextCell 5
    rect InsertCellPoint 0
    rect InsertCellPoint 1
    rect InsertCellPoint 2
    rect InsertCellPoint 3
    rect InsertCellPoint 0

vtkPolyData foregroundRect
    foregroundRect SetPoints pts1
    foregroundRect SetLines rect

vtkPolyData defaultRect
    defaultRect SetPoints pts2
    defaultRect SetLines rect

vtkPolyData backgroundRect
    backgroundRect SetPoints pts3
    backgroundRect SetLines rect

vtkPolyDataMapper2D foregroundMapper
    foregroundMapper SetInput foregroundRect

vtkPolyDataMapper2D defaultMapper
    defaultMapper SetInput defaultRect

vtkPolyDataMapper2D backgroundMapper
    backgroundMapper SetInput backgroundRect

vtkActor2D foregroundActor
    foregroundActor SetMapper foregroundMapper
    [foregroundActor GetProperty] SetColor 1 0 0
    [foregroundActor GetProperty] SetDisplayLocationToForeground

vtkActor2D defaultActor
    defaultActor SetMapper defaultMapper
    [defaultActor GetProperty] SetColor 0 1 0 

vtkActor2D backgroundActor
    backgroundActor SetMapper backgroundMapper
    [backgroundActor GetProperty] SetColor 0 0 1
    [backgroundActor GetProperty] SetDisplayLocationToBackground

# Create asphere
vtkSphereSource sphere
vtkPolyDataMapper   sphereMapper
    sphereMapper SetInput [sphere GetOutput]
    sphereMapper GlobalImmediateModeRenderingOn
vtkActor sphereActor
    sphereActor SetMapper sphereMapper

ren1 AddProp sphereActor
ren1 AddProp foregroundActor
ren1 AddProp defaultActor
ren1 AddProp backgroundActor

ren1 SetBackground 1 1 1
renWin SetSize 500 500
renWin Render

renWin SetFileName "TestFGBGPolyDataMapper2D.tcl.ppm"
#renWin SaveImageAsPPM

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .
