catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui and colors
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create pipeline
#
vtkMCubesReader reader
    reader SetFileName "$VTK_DATA/pineRoot/pine_root.tri"
vtkPolyDataMapper isoMapper
    isoMapper SetInput [reader GetOutput]
    isoMapper ScalarVisibilityOff
vtkActor isoActor
    isoActor SetMapper isoMapper
    eval [isoActor GetProperty] SetColor $raw_sienna

vtkOutlineFilter outline
    outline SetInput [reader GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    [outlineActor GetProperty] SetColor 0 0 0

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor isoActor
ren1 SetBackground 1 1 1
renWin SetSize 500 500
eval ren1 SetBackground $slate_grey

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

set cam [ren1 GetActiveCamera]
  $cam SetFocalPoint 40.6018 37.2813 50.1953
  $cam SetPosition 40.6018 -280.533 47.0172
  $cam SetClippingRange 26.1073 1305.36
  $cam SetViewAngle 20.9219
  $cam SetViewUp 0.0 0.0 1.0
renWin Render
#renWin SetFileName "pineRoot.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .
