catch {load vtktcl}
#
source ../../examplesTcl/vtkInt.tcl

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
[ren1 GetActiveCamera] Elevation 90

iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

renWin SetFileName matchingSpheres.tcl.ppm
#renWin SaveImageAsPPM

wm withdraw .
