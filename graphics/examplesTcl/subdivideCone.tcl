catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


# include get the vtk interactor ui
source $VTK_TCL/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkConeSource cone
 cone SetResolution 6
 cone CappingOff

vtkCleanPolyData clean
  clean SetInput [cone GetOutput]
  clean SetTolerance .1

vtkButterflySubdivisionFilter subdivide
  subdivide SetInput [clean GetOutput]
  subdivide SetNumberOfSubdivisions 5

vtkDataSetMapper mapper
   mapper SetInput [subdivide GetOutput]

vtkActor anActor
    anActor SetMapper mapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor anActor

vtkProperty back
  back SetDiffuseColor 0.8900 0.8100 0.3400

anActor SetBackfaceProperty back

[anActor GetProperty] SetDiffuseColor 1 .4 .3
[anActor GetProperty] SetSpecular .4
[anActor GetProperty] SetDiffuse .8
[anActor GetProperty] SetSpecularPower 40

ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

set cam1 [ren1 GetActiveCamera]

[ren1 GetActiveCamera] SetPosition -1.54037 -2.66027 -0.66041 
[ren1 GetActiveCamera] SetFocalPoint -0.231198 0.0987853 0.0350584 
[ren1 GetActiveCamera] SetViewAngle 21.4286
[ren1 GetActiveCamera] SetViewUp 0.566532 -0.0616892 -0.821728 
ren1 ResetCameraClippingRange

iren Initialize
renWin SetFileName "subdivideCone.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .

