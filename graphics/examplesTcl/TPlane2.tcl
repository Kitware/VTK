#
# This version of TPlane.tcl exercises the texture release mechanism.
#
catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin
vtkLight light
ren1 AddLight light

# create a plane source and actor
vtkPlaneSource plane
vtkPolyDataMapper  planeMapper
planeMapper SetInput [plane GetOutput]
vtkActor planeActor
planeActor SetMapper planeMapper


# load in the texture map
#
vtkTexture atext
vtkPNMReader pnmReader
pnmReader SetFileName "$VTK_DATA/masonry.ppm"
atext SetInput [pnmReader GetOutput]
atext InterpolateOn
planeActor SetTexture atext

# Add the actors to the renderer, set the background and size
ren1 AddActor planeActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 500 500

# render the image
iren Initialize
iren SetUserMethod {wm deiconify .vtkInteract}

renWin Render

set cam1 [ren1 GetActiveCamera]
$cam1 Elevation -30
$cam1 Roll -20
ren1 ResetCameraClippingRange
renWin Render

vtkTexture fooTexture
fooTexture SetInput [pnmReader GetOutput]

for { set i 0} { $i < 72} { incr i}  { 
   planeActor SetTexture fooTexture; 
   atext ReleaseGraphicsResources renWin; 
   atext Delete; 
   vtkTexture atext; 
   atext SetInput [pnmReader GetOutput]; 
   atext InterpolateOn; 
   planeActor SetTexture atext; 

   [ren1 GetActiveCamera] Azimuth 5;
   eval light SetFocalPoint [[ren1 GetActiveCamera] GetFocalPoint]
   eval light SetPosition [[ren1 GetActiveCamera] GetPosition]
   
   ren1 ResetCameraClippingRange
   renWin Render
}


#renWin SetFileName "TPlane2.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .




