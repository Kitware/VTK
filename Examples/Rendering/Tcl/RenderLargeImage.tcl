#
# This simple example shows how to render a very large image (i.e., one
# that cannot fit on the screen).
#

# We start off by loading some Tcl modules. One is the basic VTK library;
# the second is a package for rendering, and the last includes a set
# of color definitions.
#
package require vtk
package require vtkinteraction
package require vtktesting

# We'll import some data to start. Since we are using an importer, we've
# got to give it a render window and such. Note that the render window 
# size is set fairly small.
vtkRenderer ren
  ren SetBackground 0.1 0.2 0.4
vtkRenderWindow renWin
  renWin AddRenderer ren
  renWin SetSize 125 125
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

vtk3DSImporter importer
  importer SetRenderWindow renWin
  importer SetFileName "$VTK_DATA_ROOT/Data/Viewpoint/iflamigm.3ds"
  importer ComputeNormalsOn
  importer Read

# We'll set up the view we want.
#
[ren GetActiveCamera] SetPosition 0 1 0
[ren GetActiveCamera] SetFocalPoint 0 0 0
[ren GetActiveCamera] SetViewUp 0 0 1

# Let the renderer compute a good position and focal point.
#
ren ResetCamera
[ren GetActiveCamera] Dolly 1.4
ren ResetCameraClippingRange

# render the large image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
wm withdraw .

# Here we request that the large image is four times bigger than the
# renderers image.
#
vtkRenderLargeImage renderLarge
  renderLarge SetInput ren
  renderLarge SetMagnification 4

# We write out the image which causes the rendering to occur. If you
# watch your screen you will see the pieces being rendered right after 
# one another.
#
vtkTIFFWriter writer
  writer SetInput [renderLarge GetOutput]
  writer SetFileName largeImage.tif
  writer Write

