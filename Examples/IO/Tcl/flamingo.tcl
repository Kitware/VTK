# This example demonstrates the use of vtk3DSImporter.
# vtk3DSImporter is used to load 3D Studio files. Unlike writers,
# importers can load scenes (data as well as lights, cameras, actors
# etc.). Importers will either generate an instance of vtkRenderWindow
# and/or vtkRenderer or will use the ones you specify.

#
# First we include the VTK Tcl packages which will make available
# all of the vtk commands to Tcl
#
package require vtk
package require vtkinteraction

# Create the importer and read a file
vtk3DSImporter importer
  importer ComputeNormalsOn
  importer SetFileName "$VTK_DATA_ROOT/Data/iflamigm.3ds"
  importer Read
# Here we let the importer create a renderer and a render window
# for us. We could have also create and assigned those ourselves:
# vtkRenderWindow renWin
# importer SetRenderWindow renWin

# Assign an interactor.
# We have to ask the importer for it's render window.
set renWin [importer GetRenderWindow]
vtkRenderWindowInteractor iren
  iren SetRenderWindow $renWin

# Set the render window's size
$renWin SetSize 300 300

# Set some properties on the renderer.
# We have to ask the importer for it's renderer.
set ren [importer GetRenderer]
$ren SetBackground 0.1 0.2 0.4

# Position the camera:
# change view up to +z
set camera [$ren GetActiveCamera]
$camera SetPosition 0 1 0
$camera SetFocalPoint 0 0 0
$camera SetViewUp 0 0 1
# let the renderer compute good position and focal point
$ren ResetCamera
$camera Dolly 1.4
$ren ResetCameraClippingRange

# Set the user method (bound to key 'u')
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# Withdraw the default tk window
wm withdraw .

