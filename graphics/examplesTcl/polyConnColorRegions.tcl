catch {load vtktcl}
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/colors.tcl

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkSTLReader reader
  reader SetFileName "../../../vtkdata/cadPart.stl"

vtkCleanPolyData cpd
  cpd SetInput [reader GetOutput]

vtkPolyDataNormals normals
  normals SetMaximumRecursionDepth 100
  normals SetFeatureAngle 30
  normals SetInput [cpd GetOutput]

vtkPolyDataConnectivityFilter conn
  conn SetMaxRecursionDepth 1000
  conn SetInput [normals GetOutput]
  conn ColorRegionsOn
  conn SetExtractionModeToAllRegions
  conn Update

vtkDataSetMapper mapper
  mapper SetInput [conn GetOutput]
  eval mapper SetScalarRange [[conn GetOutput] GetScalarRange]

vtkActor actor
  actor SetMapper mapper

ren1 AddActor actor
[ren1 GetActiveCamera] Azimuth 30
[ren1 GetActiveCamera] Elevation 60
[ren1 GetActiveCamera] Dolly 1.2

wm withdraw .

iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

#renWin SetFileName polyConnColorRegions.tcl.ppm
#renWin SaveImageAsPPM

