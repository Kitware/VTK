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

vtkKochanekSpline aSplineX
vtkKochanekSpline aSplineY
vtkKochanekSpline aSplineZ

# generate random points
vtkPoints inputPoints
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
  glyphPoints SetInputData inputData
  glyphPoints SetSourceConnection [balls GetOutputPort]

vtkPolyDataMapper glyphMapper
  glyphMapper SetInputConnection [glyphPoints GetOutputPort]

vtkActor glyph
  glyph SetMapper glyphMapper
eval   [glyph GetProperty] SetDiffuseColor 1 0.6 0.6
  [glyph GetProperty] SetSpecular .3
  [glyph GetProperty] SetSpecularPower 30

ren1 AddActor glyph


vtkPoints points
# create a line
set tension 0
set bias 0
set continuity 0
  aSplineX SetDefaultTension $tension
  aSplineX SetDefaultBias $bias
  aSplineX SetDefaultContinuity $continuity
  aSplineY SetDefaultTension $tension
  aSplineY SetDefaultBias $bias
  aSplineY SetDefaultContinuity $continuity
  aSplineZ SetDefaultTension $tension
  aSplineZ SetDefaultBias $bias
  aSplineZ SetDefaultContinuity $continuity

vtkPolyData profileData
set numberOfOutputPoints 300
set offset 1.0
proc fit {} {
  global numberOfInputPoints numberOfOutputPoints offset
  points Reset
  for {set i 0} {$i< $numberOfOutputPoints} {incr i 1} {
      set t [expr ( $numberOfInputPoints - $offset ) / ($numberOfOutputPoints - 1) * $i]
      points InsertPoint $i [aSplineX Evaluate $t] [aSplineY Evaluate $t] [aSplineZ Evaluate $t]
  }
  profileData Modified
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
  profileTubes SetInputData profileData
  profileTubes SetRadius .005

vtkPolyDataMapper profileMapper
profileMapper SetInputConnection [profileTubes GetOutputPort]

vtkActor profile
  profile SetMapper profileMapper
eval  [profile GetProperty] SetDiffuseColor 1 1 0.7
  [profile GetProperty] SetSpecular .3
  [profile GetProperty] SetSpecularPower 30

ren1 AddActor profile
[ren1 GetActiveCamera] Dolly 1.5
ren1 ResetCamera
ren1 ResetCameraClippingRange
renWin SetSize 400 400

# render the image
#
iren Initialize
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .

proc defaults {} {
  aSplineX SetDefaultBias 0
  aSplineX SetDefaultTension 0
  aSplineX SetDefaultContinuity 0
  aSplineY SetDefaultBias 0
  aSplineY SetDefaultTension 0
  aSplineY SetDefaultContinuity 0
  aSplineZ SetDefaultBias 0
  aSplineZ SetDefaultTension 0
  aSplineZ SetDefaultContinuity 0
  fit
  renWin Render
}
proc varyBias {} {
    defaults
    for {set bias -1} { $bias <= 1 } {set bias [expr $bias + .05]} {
      aSplineX SetDefaultBias $bias
      aSplineY SetDefaultBias $bias
      aSplineZ SetDefaultBias $bias
      fit
      renWin Render
    }
}
proc varyTension {} {
    defaults
    for {set tension -1} { $tension <= 1 } {set tension [expr $tension + .05]} {
      aSplineX SetDefaultTension $tension
      aSplineY SetDefaultTension $tension
      aSplineZ SetDefaultTension $tension
      fit
      renWin Render
    }
}
proc varyContinuity {} {
    defaults
    for {set Continuity -1} { $Continuity <= 1 } {set Continuity [expr $Continuity + .05]} {
      aSplineX SetDefaultContinuity $Continuity
      aSplineY SetDefaultContinuity $Continuity
      aSplineZ SetDefaultContinuity $Continuity
      fit
      renWin Render
    }
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

proc opened {} {
    global offset

    set offset 1.0
    aSplineX ClosedOff
    aSplineY ClosedOff
    aSplineZ ClosedOff
    fit
    renWin Render
}


