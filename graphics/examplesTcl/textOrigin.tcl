catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Created oriented text
source $VTK_TCL/vtkInt.tcl

# pipeline
vtkAxes axes
    axes SetOrigin 0 0 0
vtkPolyDataMapper axesMapper
    axesMapper SetInput [axes GetOutput]
vtkActor axesActor
    axesActor SetMapper axesMapper

vtkVectorText atext
    atext SetText "Origin"
vtkPolyDataMapper textMapper
    textMapper SetInput [atext GetOutput]
vtkFollower textActor
    textActor SetMapper textMapper
    textActor SetScale 0.2 0.2 0.2
    textActor AddPosition 0 -0.1 0

# create graphics stuff
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor axesActor
ren1 AddActor textActor
[ren1 GetActiveCamera] Zoom 1.6

ren1 ResetCameraClippingRange
textActor SetCamera [ren1 GetActiveCamera]
renWin Render

ren1 ResetCameraClippingRange
renWin Render


iren SetUserMethod {wm deiconify .vtkInteract}

renWin SetFileName "textOrigin.tcl.ppm"
#renWin SaveImageAsPPM

wm withdraw .
