package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkConeSource cone
  cone SetResolution 50

vtkSphereSource sphere
  sphere SetPhiResolution 50
  sphere SetThetaResolution 50

vtkCubeSource cube
   cube SetXLength 1
   cube SetYLength 1
   cube SetZLength 1

vtkPolyDataMapper   sphereMapper
    sphereMapper SetInput [sphere GetOutput]
    sphereMapper GlobalImmediateModeRenderingOn

vtkActor sphereActor
    sphereActor SetMapper sphereMapper
[sphereActor GetProperty] SetDiffuseColor 1 .2 .4
vtkPolyDataMapper   coneMapper
    coneMapper SetInput [cone GetOutput]
    coneMapper GlobalImmediateModeRenderingOn

vtkActor coneActor
    coneActor SetMapper coneMapper
[coneActor GetProperty] SetDiffuseColor .2 .4 1

vtkPolyDataMapper   cubeMapper
    cubeMapper SetInput [cube GetOutput]
    cubeMapper GlobalImmediateModeRenderingOn

vtkActor cubeActor
    cubeActor SetMapper cubeMapper
  [cubeActor GetProperty] SetDiffuseColor .2 1 .4

#Add the actors to the renderer, set the background and size
#
sphereActor SetPosition  -5 0 0
ren1 AddActor sphereActor
coneActor SetPosition  0 0 0
ren1 AddActor coneActor
coneActor SetPosition  5 0 0
ren1 AddActor cubeActor

proc MakeText { primitive } {

  vtkTriangleFilter ${primitive}TriangleFilter
    ${primitive}TriangleFilter SetInput [${primitive} GetOutput]

  vtkMassProperties ${primitive}Mass
    ${primitive}Mass SetInput [${primitive}TriangleFilter GetOutput]

  set summary [${primitive}Mass Print]
  set startSum [string first "  VolumeX" $summary]
  set endSum [string length $summary]

  vtkVectorText ${primitive}Text
    ${primitive}Text SetText [string range $summary $startSum $endSum]

  vtkPolyDataMapper ${primitive}TextMapper
    ${primitive}TextMapper SetInput [${primitive}Text GetOutput]

  vtkActor ${primitive}TextActor
    ${primitive}TextActor SetMapper ${primitive}TextMapper
    ${primitive}TextActor SetScale .2 .2 .2
  return ${primitive}TextActor
}

ren1 AddActor [MakeText sphere]
ren1 AddActor [MakeText cube]
ren1 AddActor [MakeText cone]

eval sphereTextActor SetPosition [sphereActor GetPosition]
  sphereTextActor AddPosition -2 -1 0
eval cubeTextActor SetPosition [cubeActor GetPosition]
  cubeTextActor AddPosition -2 -1 0
eval coneTextActor SetPosition [coneActor GetPosition]
  coneTextActor AddPosition -2 -1 0

ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 786 256
# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
set cam1 [ren1 GetActiveCamera]
$cam1 Dolly 4.8
ren1 ResetCameraClippingRange

iren Initialize
proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin AddObserver AbortCheckEvent {TkCheckAbort}


# prevent the tk window from showing up then start the event loop
wm withdraw .
