package require vtktcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkUGFacetReader ugReader
    ugReader SetFileName $VTK_DATA_ROOT/Data/bolt.fac
    ugReader MergingOff

vtkPolyDataMapper   ugMapper
    ugMapper SetInput [ugReader GetOutput]
vtkActor ugActor
    ugActor SetMapper ugMapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor ugActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300

# render the image
#
set cam1 [ren1 GetActiveCamera]
$cam1 Elevation 210
$cam1 Azimuth 30
ren1 ResetCameraClippingRange

renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .
