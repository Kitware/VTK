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

vtkUGFacetReader ugReader
    ugReader SetFileName ../../../vtkdata/bolt.fac
    ugReader MergingOff

vtkPolyDataMapper   ugMapper
    ugMapper SetInput [ugReader GetOutput]
vtkActor ugActor
    ugActor SetMapper ugMapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor ugActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 500 500

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
set cam1 [ren1 GetActiveCamera]
$cam1 Elevation 210
$cam1 Azimuth 30

iren Initialize
#renWin SetFileName "ugFacet.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .
