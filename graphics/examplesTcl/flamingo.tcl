catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }
# Demonstrates the 3D Studio Importer

# get the interactor ui
source $VTK_TCL/vtkInt.tcl

vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1

vtk3DSImporter importer
  importer SetRenderWindow renWin
  importer ComputeNormalsOn
  importer SetFileName "$VTK_DATA/Viewpoint/iflamigm.3ds"
  importer Read

vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

[importer GetRenderer] SetBackground 0.1 0.2 0.4
[importer GetRenderWindow] SetSize 300 300

#
# the importer created the renderer
set renCollection [renWin GetRenderers] 
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

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize
wm withdraw .

#renWin SetFileName "flamingo.tcl.ppm"
#renWin SaveImageAsPPM
