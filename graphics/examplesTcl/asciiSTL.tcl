catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# read a vtk file
#
vtkSTLReader stla
    stla SetFileName "$VTK_DATA/Viewpoint/cow.stl"
    stla MergingOff
vtkPolyDataMapper stlaMapper
    stlaMapper SetInput [stla GetOutput]
vtkActor stlaActor
    stlaActor SetMapper stlaMapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor stlaActor
ren1 SetBackground 0.2 0.3 0.4
renWin SetSize 256 256
[ren1 GetActiveCamera] SetPosition 8.53462 -15.3133 7.36157 
[ren1 GetActiveCamera] SetFocalPoint 1.14624 0.166092 -0.45963 
[ren1 GetActiveCamera] SetViewAngle 30
[ren1 GetActiveCamera] SetViewUp -0.10844 0.406593 0.907151 
[ren1 GetActiveCamera] SetClippingRange 2 200

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

#renWin SetFileName "asciiSTL.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


