catch {load vtktcl}

# get the interactor ui
source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/colors.tcl

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

vtkPolyDataMapper glyphMapper
glyphMapper SetInput [glyphPoints GetOutput]

vtkActor glyph
  glyph SetMapper glyphMapper
eval   [glyph GetProperty] SetDiffuseColor $tomato
  [glyph GetProperty] SetSpecular .3
  [glyph GetProperty] SetSpecularPower 30

ren1 AddActor glyph


vtkFloatPoints points
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
proc fit {} {
  global numberOfInputPoints numberOfOutputPoints
  points Reset
  for {set i 0} {$i< $numberOfOutputPoints} {incr i 1} {
      set t [expr ( $numberOfInputPoints - 1.0 ) / $numberOfOutputPoints * $i]
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

vtkPolyDataMapper profileMapper
profileMapper SetInput [profileTubes GetOutput]

vtkActor profile
  profile SetMapper profileMapper
eval  [profile GetProperty] SetDiffuseColor $banana
  [profile GetProperty] SetSpecular .3
  [profile GetProperty] SetSpecularPower 30

ren1 AddActor profile
[ren1 GetActiveCamera] Dolly 1.5
renWin SetSize 500 500

# render the image
#
iren Initialize
iren SetUserMethod {wm deiconify .vtkInteract}

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

renWin SetFileName KSpline.tcl.ppm
#renWin SaveImageAsPPM
