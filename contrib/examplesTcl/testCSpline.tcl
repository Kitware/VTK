catch {load vtktcl}

# get the interactor ui
source vtkInt.tcl
source "colors.tcl"

# Now create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin



vtkMath math
set numberOfInputPoints 10

vtkCardinalSpline aSplineX
vtkCardinalSpline aSplineY
vtkCardinalSpline aSplineZ

# generate random points

vtkFloatPoints inputPoints

for {set i 0} {$i<$numberOfInputPoints} {incr i 1} {
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

vtkPolyMapper glyphMapper
glyphMapper SetInput [glyphPoints GetOutput]

vtkActor glyph
  glyph SetMapper glyphMapper
eval   [glyph GetProperty] SetDiffuseColor $tomato
  [glyph GetProperty] SetSpecular .3
  [glyph GetProperty] SetSpecularPower 30

ren1 AddActor glyph


vtkFloatPoints points
# create a line
vtkPolyData profileData

set numberOfOutputPoints 200
proc fit {} {
  global numberOfInputPoints numberOfOutputPoints
  points Reset
  for {set i 0} {$i< $numberOfOutputPoints} {incr i 1} {
      set t [expr ( $numberOfInputPoints - 1.0 ) / ( $numberOfOutputPoints - 1) * $i]
      points InsertPoint $i [aSplineX Evaluate $t] [aSplineY Evaluate $t] [aSplineZ Evaluate $t]
  }
  profileData Modified
      update
}
fit

vtkCellArray lines
  lines InsertNextCell $numberOfOutputPoints

for {set i 0} {$i< $numberOfOutputPoints} {incr i 1} {
  lines InsertCellPoint $i
}


    profileData SetPoints points
    profileData SetLines lines


vtkTubeFilter profileTubes
  profileTubes SetNumberOfSides 8
  profileTubes SetInput profileData
  profileTubes SetRadius .005

vtkPolyMapper profileMapper
profileMapper SetInput [profileTubes GetOutput]

vtkActor profile
  profile SetMapper profileMapper
eval  [profile GetProperty] SetDiffuseColor $banana
  [profile GetProperty] SetSpecular .3
  [profile GetProperty] SetSpecularPower 30

ren1 AddActor profile

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

renWin Render

proc varyLeft {} {
    for {set left -1} { $left <= 1 } {set left [expr $left + .025]} {
      aSplineX SetLeftValue $left
      aSplineY SetLeftValue $left
      aSplineZ SetLeftValue $left
      fit
      renWin Render
    }
}

proc varyRight {} {
    for {set right -1} { $right <= 1 } {set right [expr $right + .025]} {
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


# prevent the tk window from showing up then start the event loop
wm withdraw .
