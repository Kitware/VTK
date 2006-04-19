# This example shows how to combine data from both the imaging
# and graphics pipelines. The vtkMergeData filter is used to
# merge the data from each together.

package require vtk
package require vtkinteraction

# Read in an image and compute a luminance value. The image is extracted
# as a set of polygons (vtkImageDataGeometryFilter). We then will
# warp the plane using the scalar (luminance) values.
#
vtkBMPReader reader
  reader SetFileName $VTK_DATA_ROOT/Data/masonry.bmp
vtkImageLuminance luminance
  luminance SetInputConnection [reader GetOutputPort]
vtkImageDataGeometryFilter geometry
  geometry SetInputConnection [luminance GetOutputPort]
vtkWarpScalar warp
  warp SetInputConnection [geometry GetOutputPort]
  warp SetScaleFactor -0.1

# Use vtkMergeFilter to combine the original image with the warped geometry.
#
vtkMergeFilter merge
  merge SetGeometryConnection [warp GetOutputPort]
  merge SetScalarsConnection  [reader GetOutputPort]
vtkDataSetMapper mapper
  mapper SetInputConnection [merge GetOutputPort]
  mapper SetScalarRange 0 255
  mapper ImmediateModeRenderingOff
vtkActor actor
  actor SetMapper mapper

# Create renderer stuff
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor
ren1 ResetCamera
[ren1 GetActiveCamera] Azimuth 20
[ren1 GetActiveCamera] Elevation 30
ren1 SetBackground 0.1 0.2 0.4
ren1 ResetCameraClippingRange

renWin SetSize 250 250

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.4
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .


