catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Generate implicit model of a sphere
#
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

# Create renderer stuff
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create pipeline
#
vtkCylinder cylinder
  cylinder SetRadius .4
  cylinder SetCenter .1 .2 .3

vtkSampleFunction sample
    sample SetImplicitFunction cylinder
vtkContourFilter iso
    iso SetInput [sample GetOutput]
    iso SetValue 0 0.0
vtkPolyDataMapper isoMapper
    isoMapper SetInput [iso GetOutput]
    isoMapper ScalarVisibilityOff
vtkActor isoActor
    isoActor SetMapper isoMapper
    eval [isoActor GetProperty] SetColor $peacock

vtkOutlineFilter outline
    outline SetInput [sample GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    [outlineActor GetProperty] SetColor 0 0 0

vtkProperty back
eval  back SetDiffuseColor $banana

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor isoActor
isoActor SetBackfaceProperty back

ren1 SetBackground 1 1 1
[ren1 GetActiveCamera] Azimuth 30
[ren1 GetActiveCamera] Elevation 60
ren1 ResetCameraClippingRange

renWin SetSize 500 500
renWin Render

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

renWin SetFileName "implicitCylinder.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


