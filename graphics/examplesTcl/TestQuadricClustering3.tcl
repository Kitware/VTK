catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


# get the interactor ui
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

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

# create a polyline
vtkPoints points
vtkPolyData profileData

set numberOfOutputPoints 2000
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


vtkPolyDataMapper lineMapper
  lineMapper SetInput profileData
vtkActor line
  line SetMapper lineMapper
  [line GetProperty] SetDiffuseColor 0.8 0.8 1.0
ren1 AddActor line


# create a separate set of verticies.
vtkPointSource verts
  verts SetCenter 0.5 0.5 0.5
  verts SetRadius 0.3
  verts SetNumberOfPoints 50000
  verts SetDistributionToShell

vtkSphereSource sphere
  sphere SetCenter 0.5 0.57 0.5
  sphere SetRadius 0.3
  sphere SetPhiResolution 200
  sphere SetThetaResolution 200

vtkAppendPolyData appendf
  appendf AddInput profileData
  appendf AddInput [verts GetOutput]
  appendf AddInput [sphere GetOutput]

vtkQuadricClustering mesh
  mesh SetInput [appendf GetOutput]
  #mesh SetInput profileData
  mesh SetNumberOfXDivisions 20
  mesh SetNumberOfYDivisions 20
  mesh SetNumberOfZDivisions 20

vtkPolyDataMapper mapper
  mapper SetInput [mesh GetOutput]
vtkActor actor
  actor SetMapper mapper
  [actor GetProperty] SetColor 0.8 0.8 1.0
ren1 AddActor actor

vtkTubeFilter profileTubes
  profileTubes SetNumberOfSides 8
  profileTubes SetInput [mesh GetOutput]
  # profileTubes SetInput profileData
  profileTubes SetRadius .005

vtkPolyDataMapper profileMapper
  profileMapper SetInput [profileTubes GetOutput]
vtkActor profile
  profile SetMapper profileMapper
  eval  [profile GetProperty] SetDiffuseColor $banana
  [profile GetProperty] SetSpecular .3
  [profile GetProperty] SetSpecularPower 30
ren1 AddActor profile

[ren1 GetActiveCamera] Dolly 1.5
ren1 ResetCameraClippingRange

renWin SetSize 500 500

# render the image
#
iren Initialize
iren SetUserMethod {wm deiconify .vtkInteract}

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

renWin SetFileName CSpline.tcl.ppm
#renWin SaveImageAsPPM

