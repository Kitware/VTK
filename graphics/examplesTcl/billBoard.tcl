catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Create a rolling billboard - requires texture support

# Get the interactor
source $VTK_TCL/vtkInt.tcl

# load in the texture map
#
vtkPNMReader pnmReader
  pnmReader SetFileName "$VTK_DATA/billBoard.pgm"
vtkTexture atext
  atext SetInput [pnmReader GetOutput]
  atext InterpolateOn

# create a plane source and actor
vtkPlaneSource plane
plane SetPoint1 1024 0 0
plane SetPoint2 0 64 0
vtkTransformTextureCoords trans
  trans SetInput [plane GetOutput]
vtkDataSetMapper  planeMapper
  planeMapper SetInput [trans GetOutput]
vtkActor planeActor
  planeActor SetMapper planeMapper
  planeActor SetTexture atext

# Create graphics stuff
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
ren1 AddActor planeActor
ren1 SetBackground 0.1 0.2 0.4
iren SetUserMethod {wm deiconify .vtkInteract}
renWin SetSize 512 32

# Setup camera
vtkCamera camera
  camera SetClippingRange 11.8369 591.843
  camera SetFocalPoint 512 32 0
  camera SetPosition 512 32 118.369
  camera SetViewAngle 30
  camera SetDistance 118.369
  camera SetViewUp 0 1 0
ren1 SetActiveCamera camera
renWin Render

for {set i 0} {$i < 112} {incr i} {
   eval trans AddPosition 0.01 0 0
   renWin Render
}
for {set i 0} {$i < 40} {incr i} {
   eval trans AddPosition 0 0.05 0
   renWin Render
}
for {set i 0} {$i < 112} {incr i} {
   eval trans AddPosition -0.01 0 0
   renWin Render
}

#renWin SetFileName billBoard.tcl.ppm
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .





