catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Demonstrates rendering a large image

# get the interactor ui
source $VTK_TCL/vtkInt.tcl

vtkRenderer ren1
vtkRenderWindow renWin1
  renWin1 AddRenderer ren1

vtk3DSImporter importer
  importer SetRenderWindow renWin1
  importer ComputeNormalsOn
  importer SetFileName "$VTK_DATA/Viewpoint/iflamigm.3ds"
  importer Read

[importer GetRenderer] SetBackground 0.1 0.2 0.4
[importer GetRenderWindow] SetSize 125 125

#
# the importer created the renderer
set renCollection [renWin1 GetRenderers] 
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
ren1 ResetCameraClippingRange

# render the large image
#
wm withdraw .

vtkRenderLargeImage renderLarge
  renderLarge SetInput ren1
  renderLarge SetMagnification 4
  renderLarge Update

vtkImageViewer viewer
  viewer SetInput [renderLarge GetOutput]
  viewer SetColorWindow 255
  viewer SetColorLevel 127.5
  viewer Render

vtkPNMWriter writer
  writer SetFileName largeImage.tcl.ppm
  writer SetInput [renderLarge GetOutput]
#  writer Write

