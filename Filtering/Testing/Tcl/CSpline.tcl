package require vtk
package require vtkinteraction

# Now create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkMath math
set numberOfInputPoints 30

vtkCardinalSpline aSplineX
vtkCardinalSpline aSplineY
vtkCardinalSpline aSplineZ

# generate random points

vtkPoints inputPoints
for {set i 0} {$i < $numberOfInputPoints} {incr i 1} {
    set x  [math Random 0 1]
    set y  [math Random 0 1]
    set z  [math Random 0 1]
    aSplineX AddPoint $i $x
    aSplineY AddPoint $i $y
    aSplineZ AddPoint $i $z
    inputPoints InsertPoint $i $x $y $z
}

vtkPolyData inputData
  inputData SetPoints inputPoints

vtkSphereSource balls
  balls SetRadius .01
  balls SetPhiResolution 10
  balls SetThetaResolution 10

vtkGlyph3D glyphPoints
  glyphPoints SetInput inputData
  glyphPoints SetSource [balls GetOutput]

vtkPolyDataMapper glyphMapper
  glyphMapper SetInput [glyphPoints GetOutput]

vtkActor glyph
  glyph SetMapper glyphMapper
  eval   [glyph GetProperty] SetDiffuseColor 1 0.4 0.4
  [glyph GetProperty] SetSpecular .3
  [glyph GetProperty] SetSpecularPower 30

ren1 AddActor glyph

# create a polyline
vtkPoints points
vtkPolyData profileData

set numberOfOutputPoints 400
set offset 1.0
proc fit {} {
  global numberOfInputPoints numberOfOutputPoints offset
  points Reset
  for {set i 0} {$i< $numberOfOutputPoints} {incr i 1} {
      set t [expr ( $numberOfInputPoints - $offset ) / ( $numberOfOutputPoints - 1) * $i]
      points InsertPoint $i [aSplineX Evaluate $t] [aSplineY Evaluate $t] [aSplineZ Evaluate $t]
  }
  profileData Modified
}
fit

vtkCellArray lines
  lines InsertNextCell $numberOfOutputPoints

for {set i 0} {$i < $numberOfOutputPoints} {incr i 1} {
  lines InsertCellPoint $i
}
profileData SetPoints points
profileData SetLines lines

vtkTubeFilter profileTubes
  profileTubes SetNumberOfSides 8
  profileTubes SetInput profileData
  profileTubes SetRadius .005

vtkPolyDataMapper profileMapper
  profileMapper SetInput [profileTubes GetOutput]

vtkActor profile
  profile SetMapper profileMapper
  eval  [profile GetProperty] SetDiffuseColor 1 1 0.6
  [profile GetProperty] SetSpecular .3
  [profile GetProperty] SetSpecularPower 30

ren1 AddActor profile

[ren1 GetActiveCamera] Dolly 1.5
ren1 ResetCameraClippingRange

renWin SetSize 400 400

# render the image
#
iren Initialize
iren AddObserver UserEvent {wm deiconify .vtkInteract}

proc opened {} {
    global offset

    set offset 1.0
    aSplineX ClosedOff
    aSplineY ClosedOff
    aSplineZ ClosedOff
    fit
    renWin Render
}

proc varyLeft {} {
    for {set left -1} { $left <= 1 } {set left [expr $left + .05]} {
      aSplineX SetLeftValue $left
      aSplineY SetLeftValue $left
      aSplineZ SetLeftValue $left
      fit
      renWin Render
    }
}

proc varyRight {} {
    for {set right -1} { $right <= 1 } {set right [expr $right + .05]} {
      aSplineX SetRightValue $right
      aSplineY SetRightValue $right
      aSplineZ SetRightValue $right
      fit
      renWin Render
    }
}

proc constraint {value } {
  aSplineX SetLeftConstraint $value
  aSplineY SetLeftConstraint $value
  aSplineZ SetLeftConstraint $value
  aSplineX SetRightConstraint $value
  aSplineY SetRightConstraint $value
  aSplineZ SetRightConstraint $value
}

proc closed {} {
    global offset

    set offset 0.0
    aSplineX ClosedOn
    aSplineY ClosedOn
    aSplineZ ClosedOn
    fit
    renWin Render
}

# prevent the tk window from showing up then start the event loop
wm withdraw .


