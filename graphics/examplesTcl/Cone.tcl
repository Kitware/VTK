catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# user interface command widget
source $VTK_TCL/vtkInt.tcl

# create a rendering window and renderer
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin StereoCapableWindowOn  
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create an actor and give it cone geometry
vtkConeSource cone
  cone SetResolution 8
vtkPolyDataMapper coneMapper
  coneMapper SetInput [cone GetOutput]
vtkActor coneActor
  coneActor SetMapper coneMapper

# assign our actor to the renderer
ren1 AddActor coneActor

# enable user interface interactor
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .

