catch {load vtktcl}
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkPolyDataReader reader
    reader SetFileName ../../../vtkdata/spleen.color.vtk

vtkPolyDataMapper   frogMapper
    frogMapper SetInput [reader GetOutput]
vtkActor frogActor
    frogActor SetMapper frogMapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor frogActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 500 500

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.4
$cam1 Azimuth 160

iren Initialize
#renWin SetFileName "frogSpleen.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .
