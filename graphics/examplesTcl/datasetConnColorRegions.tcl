catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkSTLReader reader
  reader SetFileName "$VTK_DATA/cadPart.stl"

vtkCleanPolyData cpd
  cpd SetInput [reader GetOutput]

vtkPolyDataNormals normals
  normals SetFeatureAngle 30
  normals SetInput [cpd GetOutput]

vtkConnectivityFilter conn
  conn SetInput [normals GetOutput]
  conn ColorRegionsOn
  conn SetExtractionModeToAllRegions
  conn Update

#
# we need an explicit geometry filter to turn merging off
# if we just used a dataset mapper, the points are merged and
# the normals are not what we expect
#

vtkGeometryFilter geometry
  geometry SetInput [conn GetOutput]
  geometry MergingOff

vtkPolyDataMapper mapper
  mapper SetInput [geometry GetOutput]
  eval mapper SetScalarRange [[conn GetOutput] GetScalarRange]

vtkActor actor
  actor SetMapper mapper

ren1 AddActor actor
[ren1 GetActiveCamera] Azimuth 30
[ren1 GetActiveCamera] Elevation 60
[ren1 GetActiveCamera] Dolly 1.2
ren1 ResetCameraClippingRange

wm withdraw .

iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

#renWin SetFileName polyConnColorRegions.tcl.ppm
#renWin SaveImageAsPPM

