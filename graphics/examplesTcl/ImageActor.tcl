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

# load in the image
#
vtkPNMReader pnmReader
pnmReader SetFileName "$VTK_DATA/masonry.ppm"

vtkImageActor ia
ia SetInput [pnmReader GetOutput]

# Add the actors to the renderer, set the background and size
ren1 AddActor ia
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 500 500

# render the image
iren SetUserMethod {wm deiconify .vtkInteract}
renWin Render

set cam1 [ren1 GetActiveCamera]
$cam1 Elevation -30
$cam1 Roll -20
ren1 ResetCameraClippingRange
renWin Render

vtkWindowToImageFilter w2i
w2i SetInput renWin

vtkTIFFWriter tiff
tiff SetInput [w2i GetOutput]
tiff SetFileName ImageActor.tiff
tiff Write

# prevent the tk window from showing up then start the event loop
wm withdraw .





