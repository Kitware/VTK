package require vtk
package require vtkinteraction

set root [toplevel .top -visual {truecolor 24}]
wm title .top "superquadric viewer"
wm protocol .top WM_DELETE_WINDOW ::vtk::cb_exit

# create render window
vtkRenderWindow renWin
set ren [vtkTkRenderWidget $root.ren -width 550 -height 450 -rw renWin]
::vtk::bind_tk_render_widget $ren

# create parameter sliders
set prs [scale $root.prs -from 0 -to 3.5 -res 0.1 -orient horizontal \
	-label "phi roundness"]

set trs [scale $root.trs -from 0 -to 3.5 -res 0.1 -orient horizontal \
	-label "theta roundness"]

set thicks [scale $root.thicks -from 0.01 -to 1 -res 0.01 -orient horizontal \
	-label "thickness"]

set rframe [frame $root.rframe]
set torbut [checkbutton $rframe.torbut -text "Toroid" -variable toroid]
set texbut [checkbutton $rframe.texbut -text "Texture" -variable doTexture]

grid $ren - -sticky news
grid $rframe $thicks -sticky news  -padx 10 -ipady 5
grid $rframe -sticky news
grid $prs $trs -sticky news   -padx 10 -ipady 5
pack $torbut $texbut -padx 10 -pady 5 -ipadx 20 -ipady 5 -side right -anchor s
pack propagate $rframe no

set renWin1 [$ren GetRenderWindow]

# create pipeline
vtkSuperquadricSource squad
squad SetPhiResolution 20
squad SetThetaResolution 25

vtkPNMReader pnmReader
  pnmReader SetFileName "$VTK_DATA_ROOT/Data/earth.ppm"
vtkTexture atext
  atext SetInputConnection [pnmReader GetOutputPort]
  atext InterpolateOn

vtkAppendPolyData appendSquads
    appendSquads AddInputConnection [squad GetOutputPort]

vtkPolyDataMapper mapper
    mapper SetInputConnection [squad GetOutputPort]
    mapper ScalarVisibilityOff
vtkActor actor
    actor SetMapper mapper
    actor SetTexture atext
    eval [actor GetProperty] SetDiffuseColor 0.5 0.8 0.8
    eval [actor GetProperty] SetAmbient 0.2
    eval [actor GetProperty] SetAmbientColor 0.2 0.2 0.2


proc setTexture {actor texture win} {
    global doTexture
    if $doTexture {
	$actor SetTexture $texture
    } else {
	$actor SetTexture {}
    }
    $win Render
}

proc setPhi {squad win phi} {
    $squad SetPhiRoundness $phi
    $win Render
}

proc setTheta {squad win theta} {
    $squad SetThetaRoundness $theta
    $win Render
}

proc setThickness {squad win thickness} {
    $squad SetThickness $thickness
    $win Render
}

proc setToroid {squad scale win} {
    global toroid
    $squad SetToroidal $toroid
    if {$toroid} {
	$scale config -state normal -fg black
    } else {
	$scale config -state disabled -fg gray
    }
    $win Render
}

$prs set 1.0
$trs set 0.7
$thicks set 0.3
set toroid 1
set doTexture 0

squad SetPhiRoundness [$prs get]
squad SetThetaRoundness [$trs get]
squad SetToroidal $toroid
squad SetThickness [$thicks get]
squad SetScale 1 1 1
setTexture actor atext $renWin1

# Create renderer stuff
#
vtkRenderer ren1
ren1 SetAmbient 1 1 1
$renWin1 AddRenderer ren1


# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor
ren1 SetBackground 0.25 0.2 0.2
ren1 ResetCamera
[ren1 GetActiveCamera] Zoom 1.2
[ren1 GetActiveCamera] Elevation 40
[ren1 GetActiveCamera] Azimuth -20

# prevent the tk window from showing up then start the event loop
wm withdraw .
update

$prs   config -command "setPhi squad $renWin1"
$trs   config -command "setTheta squad $renWin1"
$thicks config -command "setThickness squad $renWin1"
$torbut config -command "setToroid squad $thicks $renWin1"
$texbut config -command "setTexture actor atext $renWin1"


