catch {load vtktcl}
# this is a tcl version of plate vibration
# get the interactor ui
source vtkInt.tcl
# First create the render master
#
vtkRenderMaster rm

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow]
set ren1   [$renWin MakeRenderer]
set iren [$renWin MakeRenderWindowInteractor]

# read a vtk file
#
vtkPolyReader plate
    plate SetFileName "../../data/plate.vtk"
    plate SetVectorsName "mode2"
vtkPolyNormals normals
    normals SetInput [plate GetOutput]
vtkWarpVector warp
    warp SetInput [normals GetOutput]
    warp SetScaleFactor 0.5
vtkVectorDot color
    color SetInput [warp GetOutput]
vtkDataSetMapper plateMapper
    plateMapper SetInput [warp GetOutput]
#    plateMapper SetInput [color GetOutput]
vtkActor plateActor
    plateActor SetMapper plateMapper

# create the outline
#
vtkOutlineFilter outline
    outline SetInput [plate GetOutput]
vtkPolyMapper spikeMapper
    spikeMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper spikeMapper
eval [outlineActor GetProperty] SetColor 0.0 0.0 0.0

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors plateActor
$ren1 AddActors outlineActor
$ren1 SetBackground 0.2 0.3 0.4
$renWin SetSize 500 500

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract}
$iren Initialize
#$renWin SetFileName "vib.tcl.ppm"
#$renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


