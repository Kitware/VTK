#
# keyframe camera
#
# Keyframes camera position, focal point, view up and view angle
# 1) Manipulate camera to a key frame and hit "p"
# 2) Continue 1)
# 3) KeyRun CameraKey 120
#    will interpolate camera properties for 120 frames
# Assumes the renderer is called ren1

# Get the KeyFrame procs
source KeyFrame.tcl

# Define the name of the key frame and the proc to call. Dummy needed
# to replace method name
KeyNew CameraKey CameraIvars ""

# Define a pick method to get camera ivar's
proc PickXYZ {} {
    upvar #0 CameraKey a
    set camera [ren1 GetActiveCamera]
    KeyAdd a "[$camera GetPosition] [$camera GetFocalPoint] [$camera GetViewUp] [$camera GetViewAngle]"
}
# Use the fast picker
vtkWorldPointPicker fastPicker
iren SetPicker fastPicker
iren SetEndPickMethod  {PickXYZ}

# Define the proc to control camera
proc CameraIvars {px py pz fx fy fz ux uy uz ang} {
  set camera [ren1 GetActiveCamera]
  $camera SetPosition $px $py $pz
  $camera SetFocalPoint $fx $fy $fz
  $camera SetViewUp $ux $uy $uz
  $camera SetViewAngle $ang
}

#
# define the Render proc for key framing the camera
#
proc KeyRender {} {
    [ren1 GetLights] InitTraversal
    set light [[ren1 GetLights] GetNextItem]
    set camera [ren1 GetActiveCamera]
    $camera SetClippingRange .1 1000
    eval $light SetPosition [$camera GetPosition]
    eval $light SetFocalPoint [$camera GetFocalPoint]
    renWin Render
}

