#
# This simple example shows how to do basic texture mapping.
#
# We start off by loading some Tcl modules. One is the basic VTK library;
# the other is a package for rendering.
#
package require vtk
package require vtkinteraction

# Load in the texture map. A texture is any unsigned char image. If it
# is not of this type, you will have to map it through a lookup table
# or by using vtkImageShiftScale.
#
vtkBMPReader bmpReader
  bmpReader SetFileName "$VTK_DATA_ROOT/Data/masonry.bmp"
vtkTexture atext
  atext SetInputConnection [bmpReader GetOutputPort]
  atext InterpolateOn

# Create a plane source and actor. The vtkPlanesSource generates
# texture coordinates.
#
vtkPlaneSource plane
vtkPolyDataMapper  planeMapper
  planeMapper SetInputConnection [plane GetOutputPort]
vtkActor planeActor
  planeActor SetMapper planeMapper
  planeActor SetTexture atext

# Create the RenderWindow, Renderer and both Actors
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
ren1 AddActor planeActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 500 500

# render the image
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render

ren1 ResetCamera
set cam1 [ren1 GetActiveCamera]
$cam1 Elevation -30
$cam1 Roll -20
ren1 ResetCameraClippingRange
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .





