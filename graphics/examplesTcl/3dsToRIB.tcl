catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Convert a 3d Studio file to Renderman RIB

# get the interactor ui
source $VTK_TCL/vtkInt.tcl

vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtk3DSImporter importer
  importer SetRenderWindow renWin
  importer ComputeNormalsOn
  importer SetFileName "$VTK_DATA/harley-d.3ds"
  importer Read

[importer GetRenderer] SetBackground 0.1 0.2 0.4
[importer GetRenderWindow] SetSize 300 300

#
# change view up to +z
#
[ren1 GetActiveCamera] SetPosition .6 -1 .5
[ren1 GetActiveCamera] SetFocalPoint 0 0 0
[ren1 GetActiveCamera] SetViewUp 0 0 1

#
# let the renderer compute good position and focal point
#
ren1 ResetCamera
[ren1 GetActiveCamera] Dolly 1.4
ren1 ResetCameraClippingRange


#
# export to rib format

if { [info command vtkRIBExporter] != "" } {
 vtkRIBExporter exporter
   exporter SetFilePrefix importExport
   exporter SetRenderWindow [importer GetRenderWindow]
   exporter BackgroundOn
   exporter Write
 }

#
# now do the normal rendering
renWin Render

renWin SetFileName "3dsToRIB.tcl.ppm"
#renWin SaveImageAsPPM

iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .
