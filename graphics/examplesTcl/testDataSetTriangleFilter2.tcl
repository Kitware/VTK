catch {load vtktcl}
if {[catch {set VTK_TCL $env(VTK_TCL)}] != 0} {set VTK_TCL "../../examplesTcl"}
if {[catch {set VTK_DATA $env(VTK_DATA)}] != 0} {set VTK_DATA "../../../vtkdata"}

# get the interactor ui
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

# Create the RenderWindow, Renderer, and RenderWindowInteractor
#
vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

# create pipeline
#
vtkDataSetReader reader
  reader SetFileName "$VTK_DATA/uGridEx.vtk"

vtkDataSetTriangleFilter tris
  tris SetInput [reader GetOutput]

vtkShrinkFilter shrink
  shrink SetInput [tris GetOutput]
  shrink SetShrinkFactor .8

vtkDataSetMapper mapper
  mapper SetInput [shrink GetOutput]
  mapper SetScalarRange 0 26
vtkActor actor
  actor SetMapper mapper

# add the actor to the renderer; set the size
#
ren1 AddActor actor
renWin SetSize 450 450
ren1 SetBackground 1 1 1

[ren1 GetActiveCamera] SetPosition -4.01115 6.03964 10.5393 
[ren1 GetActiveCamera] SetFocalPoint 1 0.525 3.025 
[ren1 GetActiveCamera] SetViewAngle 30
[ren1 GetActiveCamera] SetViewUp 0.114284 0.835731 -0.537115 
[ren1 GetActiveCamera] SetClippingRange 4.83787 17.8392 

renWin Render

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .









