catch {load vtktcl}
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/colors.tcl

vtkRenderer ren1
  ren1 SetBackground 0.8235 0.7059 0.5490

vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkSphereSource aSphere
 aSphere SetRadius .1

vtkPolyDataMapper aSphereMapper
aSphereMapper SetInput [aSphere GetOutput]
vtkActor seed
  seed SetPosition 125.6 90.5 222.678
  seed SetMapper aSphereMapper
[seed GetProperty] SetColor 1 0 0

ren1 AddActor seed

vtkSTLReader reader
  reader SetFileName ../../../vtkdata/42400-IDGH.stl

vtkCleanPolyData cpd
  cpd SetInput [reader GetOutput]

vtkPolyDataNormals normals
  normals SetMaxRecursionDepth 10
  normals SetFeatureAngle 15
  normals SetInput [cpd GetOutput]

vtkPolyDataConnectivityFilter conn
  conn SetMaxRecursionDepth 1000
  conn SetInput [normals GetOutput]
  conn SetExtractionModeToClosestPointRegion
  conn SetClosestPoint 125.6 90.5 222.678
  conn Update

vtkDataSetMapper mapper
  mapper SetInput [conn GetOutput]
  eval mapper SetScalarRange [[conn GetOutput] GetScalarRange]

vtkProperty backColor
  backColor SetColor 0.8900 0.8100 0.3400

vtkActor actor
  actor SetMapper mapper
  actor SetBackfaceProperty backColor
  [actor GetProperty] SetColor 1.0000 0.3882 0.2784

ren1 AddActor actor
[ren1 GetActiveCamera] Azimuth 30
[ren1 GetActiveCamera] Dolly 1.5

wm withdraw .

iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

renWin SetFileName polyConnClosestPoint.tcl.ppm
#renWin SaveImageAsPPM

