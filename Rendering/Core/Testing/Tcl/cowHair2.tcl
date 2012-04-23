# This differs from cowHair because it checks the "MergingOff" feature
# of vtkCleanPolyData....it should give the same result.
package require vtk
package require vtkinteraction
package require vtktesting

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# read data
#
vtkOBJReader wavefront
  wavefront SetFileName "$VTK_DATA_ROOT/Data/Viewpoint/cow.obj"
  wavefront Update

vtkConeSource cone
    cone SetResolution 6
    cone SetRadius .1

vtkTransform transform
    transform Translate 0.5 0.0 0.0

vtkTransformPolyDataFilter transformF
    transformF SetInputConnection [cone GetOutputPort]
    transformF SetTransform transform

# we just clean the normals for efficiency (keep down number of cones)
vtkCleanPolyData clean
  clean SetInputConnection [wavefront GetOutputPort]
  clean PointMergingOff

vtkHedgeHog glyph
  glyph SetInputConnection [clean GetOutputPort]
  glyph SetVectorModeToUseNormal
  glyph SetScaleFactor 0.4

vtkPolyDataMapper hairMapper
  hairMapper SetInputConnection [glyph GetOutputPort]

vtkActor hair
  hair SetMapper hairMapper

vtkPolyDataMapper cowMapper
  cowMapper SetInputConnection [wavefront GetOutputPort]

vtkActor cow
  cow SetMapper cowMapper


# Add the actors to the renderer, set the background and size
#
ren1 AddActor cow
ren1 AddActor hair
ren1 ResetCamera
[ren1 GetActiveCamera] Dolly 2
[ren1 GetActiveCamera] Azimuth 30
[ren1 GetActiveCamera] Elevation 30
ren1 ResetCameraClippingRange

eval [hair GetProperty] SetDiffuseColor $saddle_brown
eval [hair GetProperty] SetAmbientColor $thistle
eval [hair GetProperty] SetAmbient .3
eval [cow GetProperty] SetDiffuseColor $beige

renWin SetSize 320 240
ren1 SetBackground .1 .2 .4

iren Initialize
renWin Render

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .
