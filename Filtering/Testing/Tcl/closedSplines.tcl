package require vtk
package require vtkinteraction
package require vtktesting

# get the interactor ui

# Now create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkMath math
set numberOfInputPoints 30

vtkKochanekSpline aKSplineX
 aKSplineX ClosedOn
vtkKochanekSpline aKSplineY
 aKSplineY ClosedOn
vtkKochanekSpline aKSplineZ
 aKSplineZ ClosedOn

vtkCardinalSpline aCSplineX
 aCSplineX ClosedOn
vtkCardinalSpline aCSplineY
 aCSplineY ClosedOn
vtkCardinalSpline aCSplineZ
 aCSplineZ ClosedOn

# add some points
vtkPoints inputPoints
set x -1.0; set y -1.0; set z 0.0
aKSplineX AddPoint 0 $x
aKSplineY AddPoint 0 $y
aKSplineZ AddPoint 0 $z
aCSplineX AddPoint 0 $x
aCSplineY AddPoint 0 $y
aCSplineZ AddPoint 0 $z
inputPoints InsertPoint 0 $x $y $z

set x 1.0; set y -1.0; set z 0.0
aKSplineX AddPoint 1 $x
aKSplineY AddPoint 1 $y
aKSplineZ AddPoint 1 $z
aCSplineX AddPoint 1 $x
aCSplineY AddPoint 1 $y
aCSplineZ AddPoint 1 $z
inputPoints InsertPoint 1 $x $y $z

set x 1.0; set y 1.0; set z 0.0
aKSplineX AddPoint 2 $x
aKSplineY AddPoint 2 $y
aKSplineZ AddPoint 2 $z
aCSplineX AddPoint 2 $x
aCSplineY AddPoint 2 $y
aCSplineZ AddPoint 2 $z
inputPoints InsertPoint 2 $x $y $z

set x -1.0; set y 1.0; set z 0.0
aKSplineX AddPoint 3 $x
aKSplineY AddPoint 3 $y
aKSplineZ AddPoint 3 $z
aCSplineX AddPoint 3 $x
aCSplineY AddPoint 3 $y
aCSplineZ AddPoint 3 $z
inputPoints InsertPoint 3 $x $y $z

vtkPolyData inputData
  inputData SetPoints inputPoints

vtkSphereSource balls
  balls SetRadius .04
  balls SetPhiResolution 10
  balls SetThetaResolution 10

vtkGlyph3D glyphPoints
  glyphPoints SetInputData inputData
  glyphPoints SetSourceConnection [balls GetOutputPort]

vtkPolyDataMapper glyphMapper
  glyphMapper SetInputConnection [glyphPoints GetOutputPort]

vtkActor glyph
  glyph SetMapper glyphMapper
eval   [glyph GetProperty] SetDiffuseColor $tomato
  [glyph GetProperty] SetSpecular .3
  [glyph GetProperty] SetSpecularPower 30

ren1 AddActor glyph


vtkPoints Kpoints
vtkPoints Cpoints

vtkPolyData profileKData
vtkPolyData profileCData
set numberOfInputPoints  5
set numberOfOutputPoints 100
set offset 1.0
proc fit {} {
  global numberOfInputPoints numberOfOutputPoints offset
  Kpoints Reset
  Cpoints Reset
  for {set i 0} {$i< $numberOfOutputPoints} {incr i 1} {
      set t [expr ( $numberOfInputPoints - $offset ) / ($numberOfOutputPoints - 1) * $i]
      Kpoints InsertPoint $i [aKSplineX Evaluate $t] [aKSplineY Evaluate $t] [aKSplineZ Evaluate $t]
      Cpoints InsertPoint $i [aCSplineX Evaluate $t] [aCSplineY Evaluate $t] [aCSplineZ Evaluate $t]
  }
  profileKData Modified
  profileCData Modified
}
fit

vtkCellArray lines
  lines InsertNextCell $numberOfOutputPoints

for {set i 0} {$i< $numberOfOutputPoints} {incr i 1} {
  lines InsertCellPoint $i
}

profileKData SetPoints Kpoints
profileKData SetLines lines
profileCData SetPoints Cpoints
profileCData SetLines lines


vtkTubeFilter profileKTubes
  profileKTubes SetNumberOfSides 8
  profileKTubes SetInputData profileKData
  profileKTubes SetRadius .01

vtkPolyDataMapper profileKMapper
profileKMapper SetInputConnection [profileKTubes GetOutputPort]

vtkActor profileK
  profileK SetMapper profileKMapper
eval  [profileK GetProperty] SetDiffuseColor $banana
  [profileK GetProperty] SetSpecular .3
  [profileK GetProperty] SetSpecularPower 30

ren1 AddActor profileK

vtkTubeFilter profileCTubes
  profileCTubes SetNumberOfSides 8
  profileCTubes SetInputData profileCData
  profileCTubes SetRadius .01

vtkPolyDataMapper profileCMapper
profileCMapper SetInputConnection [profileCTubes GetOutputPort]

vtkActor profileC
  profileC SetMapper profileCMapper
eval  [profileC GetProperty] SetDiffuseColor $peacock
  [profileC GetProperty] SetSpecular .3
  [profileC GetProperty] SetSpecularPower 30

ren1 AddActor profileC
ren1 ResetCamera
[ren1 GetActiveCamera] Dolly 1.5
ren1 ResetCameraClippingRange

renWin SetSize 300 300

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


