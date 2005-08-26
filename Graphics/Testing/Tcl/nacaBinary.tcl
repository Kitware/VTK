package require vtk
package require vtkinteraction

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkEnSightGoldBinaryReader reader
   reader SetCaseFileName "$VTK_DATA_ROOT/Data/EnSight/naca.bin.case"
   reader SetTimeValue 3
   reader Update


vtkLookupTable lut
 lut SetHueRange 0.667 0.0
 lut SetTableRange 0.636 1.34


vtkDataSetMapper blockMapper0
   blockMapper0 SetInput [reader GetOutput 0]
vtkActor blockActor0
   blockActor0 SetMapper blockMapper0

vtkDataSetMapper blockMapper1
   blockMapper1 SetInput [reader GetOutput 1]
vtkActor blockActor1
   blockActor1 SetMapper blockMapper1


vtkDataSetMapper blockMapper2
   blockMapper2 SetInput [reader GetOutput 2]
vtkActor blockActor2
   blockActor2 SetMapper blockMapper2


ren1 AddActor blockActor0
ren1 AddActor blockActor1
ren1 AddActor blockActor2

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
