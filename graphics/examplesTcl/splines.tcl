catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


# get the interactor ui
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

# Now create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderer ren2
vtkRenderer ren3
vtkRenderer ren4
vtkRenderer ren5
vtkRenderer ren6
vtkRenderWindow renWin
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkMath math
set numberOfInputPoints 7

vtkCardinalSpline spline1
  spline1 SetLeftConstraint 1
  spline1 SetLeftValue 0
  spline1 SetRightConstraint 1
  spline1 SetRightValue 1
vtkCardinalSpline spline2
  spline2 SetLeftConstraint 2
  spline2 SetLeftValue 0
  spline2 SetRightConstraint 2
  spline2 SetRightValue 1
vtkCardinalSpline spline3
  spline3 SetLeftConstraint 3
  spline3 SetLeftValue 0
  spline3 SetRightConstraint 3
  spline3 SetRightValue 1

vtkKochanekSpline spline4
  spline4 SetLeftConstraint 1
  spline4 SetLeftValue 0
  spline4 SetRightConstraint 1
  spline4 SetRightValue 1
  spline4 SetDefaultTension 1
vtkKochanekSpline spline5
  spline5 SetLeftConstraint 2
  spline5 SetLeftValue 0
  spline5 SetRightConstraint 2
  spline5 SetRightValue 1
  spline5 SetDefaultTension 1
vtkKochanekSpline spline6
  spline6 SetLeftConstraint 3
  spline6 SetLeftValue 0
  spline6 SetRightConstraint 3
  spline6 SetRightValue 1
  spline6 SetDefaultTension 1

vtkPoints inputPoints
for {set i 0} {$i < $numberOfInputPoints} {incr i 1} {
    set x  [expr 2.0 * 3.1415927 / ($numberOfInputPoints - 1) * $i]
    set y  [expr cos($x)]
    inputPoints InsertPoint $i $x $y 0
}

vtkPolyData inputData
  inputData SetPoints inputPoints

vtkSphereSource balls
  balls SetRadius .08
  balls SetPhiResolution 10
  balls SetThetaResolution 10

vtkGlyph3D glyphPoints
  glyphPoints SetInput inputData
  glyphPoints SetSource [balls GetOutput]

vtkPolyDataMapper glyphMapper
  glyphMapper SetInput [glyphPoints GetOutput]

vtkActor glyph
  glyph SetMapper glyphMapper
  eval   [glyph GetProperty] SetDiffuseColor $tomato
  [glyph GetProperty] SetSpecular .3
  [glyph GetProperty] SetSpecularPower 30

for {set s 1} {$s <= 6} {incr s} {
    for {set i 0} {$i < $numberOfInputPoints} {incr i 1} {
	set x  [expr 2.0 * 3.1415927 / ($numberOfInputPoints - 1) * $i]
	set y  [expr cos($x)]
	spline$s AddPoint $x $y
    }

    # create a polyline
    vtkPoints points$s
    vtkPolyData profileData$s

    set numberOfOutputPoints 40
    proc fit {} {
	global numberOfInputPoints numberOfOutputPoints s
	points$s Reset
	for {set i 0} {$i< $numberOfOutputPoints} {incr i 1} {
	    set t [expr 2.0 * 3.1415927 / ( $numberOfOutputPoints - 1) * $i]
	    points$s InsertPoint $i $t [spline$s Evaluate $t] 0
	}
	profileData$s Modified
    }
    fit

    vtkCellArray lines$s
    lines$s InsertNextCell $numberOfOutputPoints

    for {set i 0} {$i < $numberOfOutputPoints} {incr i 1} {
	lines$s InsertCellPoint $i
    }
    profileData$s SetPoints points$s
    profileData$s SetLines lines$s

    vtkTubeFilter profileTubes$s
      profileTubes$s SetNumberOfSides 8
      profileTubes$s SetInput profileData$s
      profileTubes$s SetRadius .04

    vtkPolyDataMapper profileMapper$s
      profileMapper$s SetInput [profileTubes$s GetOutput]

    vtkActor profile$s
      profile$s SetMapper profileMapper$s
    eval  [profile$s GetProperty] SetDiffuseColor $banana
    [profile$s GetProperty] SetSpecular .3
    [profile$s GetProperty] SetSpecularPower 30

    ren$s AddActor profile$s
    ren$s AddActor glyph
    [ren$s GetActiveCamera] Dolly 2.0
    ren$s ResetCameraClippingRange
    renWin AddRenderer ren$s
}

ren1 SetViewport 0 .66 .5 1
ren2 SetViewport 0 .33 .5 .66
ren3 SetViewport 0 0  .5 .33
ren4 SetViewport .5 .66 1 1
ren5 SetViewport .5 .33 1 .66
ren6 SetViewport .5  0  1 .33
renWin SetSize 500 500

# render the image
#
iren Initialize
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .

