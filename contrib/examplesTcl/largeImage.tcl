catch {load vtktcl}
# Demonstrates rendering a large image

# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

vtkRenderer ren1
vtkRenderWindow renWin1
  renWin1 AddRenderer ren1

vtk3DSImporter importer
  importer SetRenderWindow renWin1
  importer ComputeNormalsOn
  importer SetFileName "../../../vtkdata/Viewpoint/iflamigm.3ds"
  importer Read

vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin1

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
[$ren GetActiveCamera] ComputeViewPlaneNormal
[$ren GetActiveCamera] SetViewUp 0 0 1

#
# let the renderer compute good position and focal point
#
$ren ResetCamera
[$ren GetActiveCamera] Dolly 1.4

# render the large image
#
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize
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

