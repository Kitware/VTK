catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


# demostrates the features of the vtkRenderWindowInteractor
# with a tcl interface.



# get the interactor ui
source $VTK_TCL/vtkInt.tcl




# set up some strings for help.
set CameraHelp {
Button 1 - rotate
Button 2 - pan 
Button 3 - zoom
ctrl-Button 1 - spin}

set ActorHelp {
Button 1 - rotate
Button 2 - pan
Button 3 - scale
ctrl-Button 1 - spin
ctrl-Button 2 - dolly
}

# -----  Interface ----

wm withdraw .
toplevel .top 


vtkTkRenderWidget .top.r1 -width 400 -height 400 
set controlFm [frame .top.f2]
pack $controlFm -side left -padx 3 -pady 3 -fill both -expand f
pack .top.r1 -side left -padx 3 -pady 3 -fill both -expand t




radiobutton $controlFm.camera -text "Camera Mode <c>" -variable actorMode \
  -value "camera" -anchor nw -command {changeActorMode}
radiobutton $controlFm.actor -text "Object Mode <o>" -variable actorMode \
  -value "actor" -anchor nw -command {changeActorMode}
set actorMode camera

proc changeActorMode {} {
   global actorMode
   if {$actorMode == "actor"} {
      iren SetActorModeToActor
   } else {
      iren SetActorModeToCamera
   }
}

label $controlFm.space1 -text "    "

radiobutton $controlFm.joystick -text "Joystick Mode <j>" \
  -variable trackballMode -value joystick -anchor nw \
  -command {changeTrackballMode}
radiobutton $controlFm.trackball -text "Trackball Mode <t>" \
  -variable trackballMode -value trackball -anchor nw \
  -command {changeTrackballMode}
set trackballMode joystick

proc changeTrackballMode {} {
   global trackballMode
   if {$trackballMode == "trackball"} {
      iren SetTrackballModeToTrackball
   } else {
      iren SetTrackballModeToJoystick
   }
}


label $controlFm.space2 -text "        "

label $controlFm.help -text $CameraHelp  -justify left




pack $controlFm.camera $controlFm.actor $controlFm.space1 \
  $controlFm.joystick $controlFm.trackball $controlFm.space2 \
  $controlFm.help -side top -fill x


# set up the model

set renWin [.top.r1 GetRenderWindow]

vtkRenderer ren1
  $renWin AddRenderer ren1

vtk3DSImporter importer
  importer SetRenderWindow $renWin
  importer ComputeNormalsOn
  importer SetFileName "$VTK_DATA/Viewpoint/iflamigm.3ds"
  importer Read

# set up call back to change mode radio buttons
vtkRenderWindowInteractor iren
  iren SetRenderWindow $renWin
  iren SetCameraModeMethod {
     global actorMode controlFm
     set actorMode camera
     $controlFm.help configure -text $CameraHelp
     update
  }
  iren SetActorModeMethod {
     global actorMode controlFm
     set actorMode actor
     $controlFm.help configure -text $ActorHelp
     update
  }
  iren SetJoystickModeMethod {
     global trackballMode
     set trackballMode "joystick"
     update
  }
  iren SetTrackballModeMethod {
     global trackballMode
     set trackballMode trackball
     update
  }


[importer GetRenderer] SetBackground 0.1 0.2 0.4
[importer GetRenderWindow] SetSize 400 400

#
# the importer created the renderer
set renCollection [$renWin GetRenderers] 
$renCollection InitTraversal
set ren [$renCollection GetNextItem]

#
# change view up to +z
#
[$ren GetActiveCamera] SetPosition 0 1 0
[$ren GetActiveCamera] SetFocalPoint 0 0 0
[$ren GetActiveCamera] SetViewUp 0 0 1

#
# let the renderer compute good position and focal point
#
$ren ResetCamera
[$ren GetActiveCamera] Dolly 1.4

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize
wm withdraw .

