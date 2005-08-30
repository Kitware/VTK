package require vtk
package require vtkinteraction

vtkMath math
math RandomSeed 22

vtkParallelFactory pf
pf RegisterFactory pf

vtkPLOT3DReader pl3d
pl3d SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
pl3d SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
pl3d SetScalarFunctionNumber 100

vtkDataSetTriangleFilter dst
dst SetInputConnection [pl3d GetOutputPort]

vtkExtractUnstructuredGridPiece extract
extract SetInputConnection [dst GetOutputPort]

vtkContourFilter cf
cf SetInputConnection [extract GetOutputPort]
cf SetValue 0 0.24

vtkPolyDataNormals pdn
pdn SetInputConnection [cf GetOutputPort]

vtkPieceScalars ps
ps SetInputConnection [pdn GetOutputPort]

vtkPolyDataMapper mapper
mapper SetInputConnection [ps GetOutputPort]
mapper SetNumberOfPieces 3

vtkActor actor
actor SetMapper mapper

vtkRenderer ren
ren AddActor actor

ren ResetCamera
set camera [ren GetActiveCamera]
#$camera SetPosition 68.1939 -23.4323 12.6465
#$camera SetViewUp 0.46563 0.882375 0.0678508  
#$camera SetFocalPoint 3.65707 11.4552 1.83509 
#$camera SetClippingRange 59.2626 101.825 

vtkRenderWindow renWin
renWin AddRenderer ren

vtkRenderWindowInteractor iren
iren SetRenderWindow renWin
iren Initialize
iren AddObserver UserEvent {wm deiconify .vtkInteract}

wm withdraw .

