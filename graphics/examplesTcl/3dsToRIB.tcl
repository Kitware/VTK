catch {load vtktcl}
# Convert a 3d Studio file to Renderman RIB

# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1

vtk3DSImporter importer
  importer SetRenderWindow renWin
  importer ComputeNormalsOn
  importer SetFileName "../../../vtkdata/Viewpoint/iflamigm.3ds"
  importer Read

[importer GetRenderer] SetBackground 0.1 0.2 0.4
[importer GetRenderWindow] SetSize 300 300

#
# change view up to +z
#
[ren1 GetActiveCamera] SetPosition 0 1 0
[ren1 GetActiveCamera] SetFocalPoint 0 0 0
[ren1 GetActiveCamera] ComputeViewPlaneNormal
[ren1 GetActiveCamera] SetViewUp 0 0 1

#
# let the renderer compute good position and focal point
#
ren1 ResetCamera
[ren1 GetActiveCamera] Dolly 1.4

#
# export to rib format
vtkRIBExporter exporter
  exporter SetFilePrefix importExport
  exporter SetRenderWindow [importer GetRenderWindow]
  exporter BackgroundOn
  exporter Write

exit
