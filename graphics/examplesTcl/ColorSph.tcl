catch {load vtktcl}
# Example demonstrates use of vtkCastToConcrete
#
source ../../examplesTcl/vtkInt.tcl

vtkSphereSource sphere
    sphere SetPhiResolution 12
    sphere SetThetaResolution 12

vtkElevationFilter colorIt
  colorIt SetInput [sphere GetOutput]
  colorIt SetLowPoint 0 0 -1
  colorIt SetHighPoint 0 0 1

vtkCastToConcrete cast
  cast SetInput [colorIt GetOutput]

vtkPolyDataMapper mapper
  mapper SetInput [cast GetPolyDataOutput] 

vtkActor actor
  actor SetMapper mapper

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor actor
ren1 SetBackground 1 1 1
renWin SetSize 400 400
[ren1 GetActiveCamera] Zoom 1.4

iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

#renWin SetFileName ColorSph.tcl.ppm
#renWin SaveImageAsPPM

wm withdraw .
