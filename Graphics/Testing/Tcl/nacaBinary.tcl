package require vtk
package require vtkinteraction

# we need to use composite data pipeline with multiblock datasets
vtkAlgorithm alg
vtkCompositeDataPipeline pip
alg SetDefaultExecutivePrototype pip
pip Delete

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkEnSightGoldBinaryReader reader
   reader SetCaseFileName "$VTK_DATA_ROOT/Data/EnSight/naca.bin.case"
   reader SetTimeValue 3

vtkLookupTable lut
 lut SetHueRange 0.667 0.0
 lut SetTableRange 0.636 1.34

vtkGeometryFilter geom
geom SetInputConnection [reader GetOutputPort]

vtkHierarchicalPolyDataMapper blockMapper0
   blockMapper0 SetInputConnection [geom GetOutputPort]

vtkActor blockActor0
   blockActor0 SetMapper blockMapper0

ren1 AddActor blockActor0

ren1 ResetCamera
set cam1 [ren1 GetActiveCamera]
$cam1 SetFocalPoint 0 0 0
$cam1 ParallelProjectionOff
$cam1 Zoom 70
$cam1 SetViewAngle 1.0


renWin SetSize 400 400
iren Initialize
iren AddObserver UserEvent {wm deiconify .vtkInteract}
wm withdraw .

alg SetDefaultExecutivePrototype {}