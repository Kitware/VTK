catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

#
source $VTK_TCL/vtkInt.tcl

vtkSphereSource sphere1
    sphere1 SetPhiResolution 0
    sphere1 SetThetaResolution 0
    sphere1 SetStartPhi 0
    sphere1 SetEndPhi 90

vtkSphereSource sphere2
    sphere2 SetPhiResolution 0
    sphere2 SetThetaResolution 0
    sphere2 SetStartPhi 90
    sphere2 SetEndPhi 180

vtkPolyDataMapper mapper1
  mapper1 SetInput [sphere1 GetOutput] 

vtkActor actor1
  actor1 SetMapper mapper1

vtkPolyDataMapper mapper2
  mapper2 SetInput [sphere2 GetOutput] 

vtkActor actor2
  actor2 SetMapper mapper2

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor actor1
ren1 AddActor actor2

ren1 SetBackground 1 1 1
[ren1 GetActiveCamera] SetPosition 0.125 3.08564 6.8515e-16 
[ren1 GetActiveCamera] SetFocalPoint 0.125 -1.49012e-08 0 
[ren1 GetActiveCamera] SetViewAngle 30
[ren1 GetActiveCamera] SetViewUp 0 1 0 
[ren1 GetActiveCamera] SetClippingRange 2.05978 4.38443 


iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

renWin SetFileName matchingSpheres.tcl.ppm
#renWin SaveImageAsPPM

wm withdraw .
