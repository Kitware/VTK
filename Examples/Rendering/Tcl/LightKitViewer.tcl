# This example builds a user interface to set the parameters for
# a vtkLightKit object.  The default parameters for a LightKit
# are pretty reasonable, but moving the lights around may help
# a user light a particular object.

package require vtk
package require vtkinteraction
package require vtktesting

# demonstrate LightKit functionality.

set root [toplevel .top -visual {truecolor 24}]
wm title .top "LightKit Viewer"
wm protocol .top WM_DELETE_WINDOW exit
wm withdraw .

vtkRenderWindow renWin
set renWidget [vtkTkRenderWidget $root.ren -width 400 -height 400 -rw renWin]
::vtk::bind_tk_render_widget $renWidget

vtkRenderer ren1
renWin AddRenderer ren1

vtkLightKit lightKit
lightKit AddLightsToRenderer ren1

set TitleFont {-size 12 -family Helvetica -weight bold}


# Key Light panel
set keyfrm [frame $root.keyfrm]
set keylbl [label $keyfrm.lbl -text "Key Light" -anchor w  -font $TitleFont]
set keywarmlbl [label $keyfrm.warmlbl -text "Warmth" -anchor se]
set keywarm [scale $keyfrm.warm -from 0 -to 1 -res 0.01 -orient horizontal]

set keyelevlbl [label $keyfrm.elevlbl -text "Elevation" -anchor se]
set keyelev [scale $keyfrm.elev -from 0 -to 90 -orient horizontal]

set keyazimlbl [label $keyfrm.azimlbl -text "Azimuth" -anchor se]
set keyazim [scale $keyfrm.azim -from -90 -to 90 -orient horizontal]

set keyintlbl [label $keyfrm.intlbl -text "Intensity" -anchor se]
set keyint [scale $keyfrm.int -from 0 -to 2 -orient horizontal -res 0.05]

grid $keylbl - -sticky news
grid $keywarmlbl $keywarm -sticky news
grid $keyintlbl $keyint -sticky news

grid $keyelevlbl $keyelev -sticky news
grid $keyazimlbl $keyazim -sticky news

# Fill Light Panel
set fillfrm [frame $root.fillfrm]
set filllbl [label $fillfrm.lbl -text "Fill Light" -anchor w -font $TitleFont]

set fillwarmlbl [label $fillfrm.warmlbl -text "Warmth" -anchor se]
set fillwarm [scale $fillfrm.warm -from 0 -to 1 -res 0.01 -orient horizontal]

set fillelevlbl [label $fillfrm.elevlbl -text "Elevation" -anchor se]
set fillelev [scale $fillfrm.elev -from -90 -to 10 -orient horizontal]

set fillazimlbl [label $fillfrm.azimlbl -text "Azimuth" -anchor se]
set fillazim [scale $fillfrm.azim -from -90 -to 90 -orient horizontal]

set fillratlbl [label $fillfrm.ratlbl -text "K:F Ratio" -anchor se]
set fillrat [scale $fillfrm.rat -from 1 -to 15 -res 0.1 -orient horizontal]

grid $filllbl - -sticky news
grid $fillwarmlbl $fillwarm -sticky news
grid $fillratlbl $fillrat -sticky news

grid $fillelevlbl $fillelev -sticky news
grid $fillazimlbl $fillazim -sticky news

# Back Light panel
set backfrm [frame $root.backfrm]
set backlbl [label $backfrm.lbl -text "Back Light" -anchor w -font $TitleFont]
set backwarmlbl [label $backfrm.warmlbl -text "Warmth" -anchor se]
set backwarm [scale $backfrm.warm -from 0 -to 1 -res 0.01 -orient horizontal]

set backelevlbl [label $backfrm.elevlbl -text "Elevation" -anchor se]
set backelev [scale $backfrm.elev -from -45 -to 45 -orient horizontal]

set backazimlbl [label $backfrm.azimlbl -text "Azimuth" -anchor se]
set backazim [scale $backfrm.azim -from 60 -to 170 -orient horizontal]

set backratlbl [label $backfrm.ratlbl -text "K:B Ratio" -anchor se]
set backrat [scale $backfrm.rat -from 1 -to 15 -res 0.1 -orient horizontal]

grid $backlbl - -sticky news
grid $backwarmlbl $backwarm -sticky news
grid $backratlbl $backrat -sticky news

grid $backelevlbl $backelev -sticky news
grid $backazimlbl $backazim -sticky news

# HeadLight panel
set headfrm [frame $root.headfrm]
set headlbl [label $headfrm.lbl -text "Head Light" -anchor w  -font $TitleFont]
set headwarmlbl [label $headfrm.warmlbl -text "Warmth" -anchor se]
set headwarm [scale $headfrm.warm -from 0 -to 1 -res 0.01 -orient horizontal]

set headratlbl [label $headfrm.ratlbl -text "K:H Ratio" -anchor se]
set headrat [scale $headfrm.rat -from 1 -to 15 -res 0.1 -orient horizontal]


set f1 [canvas $headfrm.f1 -border 1 -relief raised -width 0 -height 0]
set f2 [canvas $headfrm.f2 -border 1 -relief raised -width 0 -height 0]

set maintlumlbl [label $headfrm.maintlumlbl -text "Maintain Luminance" -anchor w  -font $TitleFont]
set maintlum [checkbutton $headfrm.maintlum -variable MaintainLuminance]

set butfrm [frame $headfrm.butfrm]
set helpbut [button $butfrm.helpbut -text "Help" -command showHelp]
set quitbut [button $butfrm.quitbut -text "Quit" -command exit]
pack $helpbut -side left -expand  true -fill x
pack $quitbut -side right -expand true -fill x

grid $headlbl - -sticky nw
grid $headwarmlbl $headwarm -sticky news
grid $headratlbl $headrat -sticky news
grid $f1 - -sticky ew
grid $maintlumlbl - -sticky news
grid $maintlum - -sticky news
grid $f2 - -sticky ew
grid $butfrm - -sticky ew
grid [frame $headfrm.f0] -sticky news

grid rowconfigure $headfrm 7 -weight 1

set f0 [canvas $root.f0 -border 1 -relief raised -width 0 -height 0]
set f1 [canvas $root.f1 -border 1 -relief raised -width 0 -height 0]
set f2 [canvas $root.f2 -border 1 -relief raised -width 0 -height 0]
set f3 [canvas $root.f3 -border 1 -relief raised -width 0 -height 0]


grid $renWidget - - - - - - -sticky news
grid $keyfrm $f0 $fillfrm $f1 $backfrm $f2 $headfrm -sticky news

vtkSuperquadricSource squad
   squad SetPhiResolution 20
   squad SetThetaResolution 25
squad SetPhiRoundness 1.5
squad SetThickness 0.43
squad SetThetaRoundness 0.7
squad ToroidalOn

vtkAppendPolyData appendSquads
    appendSquads AddInputConnection [squad GetOutputPort]

vtkPolyDataMapper mapper
    mapper SetInputConnection [squad GetOutputPort]
    mapper ScalarVisibilityOff

vtkActor actor
    actor SetMapper mapper
    eval [actor GetProperty] SetDiffuseColor 1 1 1
    eval [actor GetProperty] SetSpecularColor 1 1 1
    eval [actor GetProperty] SetSpecularPower 20
    eval [actor GetProperty] SetSpecular 0.4
    eval [actor GetProperty] SetDiffuse 0.7

ren1 AddActor actor
ren1 SetBackground 0.1 0.1 0.15
ren1 ResetCamera
[ren1 GetActiveCamera] Zoom 1.5
[ren1 GetActiveCamera] Elevation 40
[ren1 GetActiveCamera] Azimuth -20

proc SetLightWarmth {lt x} {
    lightKit Set${lt}LightWarmth $x
    renWin Render
}

proc SetHeadLightWarmth {x} {
    lightKit SetHeadLightWarmth $x
    renWin Render
}

proc SetLightElevation {lt x} {
    lightKit Set${lt}LightElevation $x
    renWin Render
}

proc SetLightAzimuth {lt x} {
    lightKit Set${lt}LightAzimuth $x
    renWin Render
}

proc SetKeyToFillRatio {x} {
    lightKit SetKeyToFillRatio $x
    renWin Render
}

proc SetKeyToBackRatio {x} {
    lightKit SetKeyToBackRatio $x
    renWin Render
}

proc SetKeyToHeadRatio {x} {
    lightKit SetKeyToHeadRatio $x
    renWin Render
}

proc SetKeyLightIntensity {x} {
    lightKit SetKeyLightIntensity $x
    renWin Render
}

proc SetMaintainLuminance {} {
    global MaintainLuminance
    lightKit SetMaintainLuminance $MaintainLuminance
    renWin Render
}

# get defaults directly from the LightKit

$keywarm set [lightKit GetKeyLightWarmth]
$keyelev set [lightKit GetKeyLightElevation]
$keyazim set [lightKit GetKeyLightAzimuth]
$keyint set [lightKit GetKeyLightIntensity]

$fillwarm set [lightKit GetFillLightWarmth]
$fillelev set [lightKit GetFillLightElevation]
$fillazim set [lightKit GetFillLightAzimuth]
$fillrat set [lightKit GetKeyToFillRatio]

$backwarm set [lightKit GetBackLightWarmth]
$backrat set [lightKit GetKeyToBackRatio]
$backelev set [lightKit GetBackLightElevation]
$backazim set [lightKit GetBackLightAzimuth]

$headwarm set [lightKit GetHeadLightWarmth]
$headrat  set [lightKit GetKeyToHeadRatio]

set MaintainLuminance [lightKit GetMaintainLuminance]

$keywarm config -command "SetLightWarmth Key"
$keyelev config -command "SetLightElevation Key"
$keyazim config -command "SetLightAzimuth Key"
$keyint  config -command "SetKeyLightIntensity"

$fillwarm config -command "SetLightWarmth Fill"
$fillelev config -command "SetLightElevation Fill"
$fillazim config -command "SetLightAzimuth Fill"
$fillrat  config -command "SetKeyToFillRatio"

$backwarm config -command "SetLightWarmth Back"
$backrat  config -command "SetKeyToBackRatio"
$backelev config -command "SetLightElevation Back"
$backazim config -command "SetLightAzimuth Back"

$headwarm config -command "SetHeadLightWarmth"
$headrat  config -command "SetKeyToHeadRatio"

$maintlum config -command SetMaintainLuminance

proc showHelp {} {

set helpfrm .helpfrm

if {[winfo exists $helpfrm]} {
    wm deiconify $helpfrm
    raise $helpfrm
    return
}

toplevel $helpfrm
wm title $helpfrm "Help for LightKitViewer"
set msg [label $helpfrm.msg -justify left \
	-wraplength 350 -font {-weight normal -family Helvetica}]
$msg config -text \
{The key light is the dominant light in the scene, located in front of\
and above the object. The fill light is usually positioned to shine\
on the parts of the object not illuminated by the key light.  The headlight\
reduces the contrast of the scene by shining from the camera position.

Fill, back, and headlights are dimmer than the key light; K:F K:B and K:H \
control the key-to-fill, key-to-back and key-to-headlight intensity\
respectively. The key light intensity, in effect, controls the entire\
scene brightness level.

Lights always shine on the point where the camera looking, but their angles\
can be adjusted.  The position of the key, back, and fill lights are \
controlled by their elevation (latitude) and azimuth (longitude) \
expressed in degrees. Both angles are specified with respect to the camera.\
(elevation=0, azimuth=0) is located at the camera (a headlight).\
(elevation=90, azimuth=0) is directly over the object\
shining down. Negative azimuth angles position a light to the left of the\
camera, positive values right.

The back light is actually two lights, one on the left and one on the\
right.  The two lights have the same elevation, but opposite azimuth.\
When you adjust the angle of the back light, both lights are moved\
automatically for you.

The warmth parameter controls the color of the lights: 0.5 is white, 0 is\
cold blue, and 1 is deep sunset red.  Use extreme colors with restraint.

If MaintainLuminance is set, the\
brightness of the lights is scaled to compensate for their reduced\
perceptual brightness.}

pack $msg -ipadx 5 -ipady 5
}

update


