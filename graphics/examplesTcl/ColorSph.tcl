catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Example demonstrates use of abstract vtkDataSetToDataSetFilter
# (i.e., vtkElevationFilter - an abstract filter)
#
source $VTK_TCL/vtkInt.tcl

vtkSphereSource sphere
    sphere SetPhiResolution 12
    sphere SetThetaResolution 12

vtkElevationFilter colorIt
  colorIt SetInput [sphere GetOutput]
  colorIt SetLowPoint 0 0 -1
  colorIt SetHighPoint 0 0 1

vtkPolyDataMapper mapper
  mapper SetInput [colorIt GetPolyDataOutput] 

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
