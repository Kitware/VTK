package require vtk
package require vtkinteraction

#
# Texture a sphere.
#

# renderer and interactor
vtkRenderer ren

vtkRenderWindow renWin
renWin AddRenderer ren

vtkRenderWindowInteractor iren
iren SetRenderWindow renWin

# read the volume
vtkJPEGReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/beach.jpg"

#---------------------------------------------------------
# Do the surface rendering
vtkSphereSource sphereSource
sphereSource SetRadius 100

vtkTextureMapToSphere textureSphere
textureSphere SetInputConnection [sphereSource GetOutputPort]

vtkStripper sphereStripper
sphereStripper SetInputConnection [textureSphere GetOutputPort]
sphereStripper SetMaximumLength 5

vtkPolyDataMapper sphereMapper
sphereMapper SetInputConnection [sphereStripper GetOutputPort]
sphereMapper ScalarVisibilityOff

vtkTexture sphereTexture
sphereTexture SetInputConnection [reader GetOutputPort]

vtkProperty sphereProperty
#sphereProperty BackfaceCullingOn

vtkActor sphere
sphere SetMapper sphereMapper
sphere SetTexture sphereTexture
sphere SetProperty sphereProperty

#---------------------------------------------------------
ren AddViewProp sphere

set camera [ren GetActiveCamera]
$camera SetFocalPoint 0 0 0
$camera SetPosition 100 400 -100
$camera SetViewUp 0 0 -1

ren ResetCameraClippingRange

renWin Render

#---------------------------------------------------------
# test-related code
proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin AddObserver AbortCheckEvent {TkCheckAbort}

iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

wm withdraw .

