catch {load vtktcl}
# this is a tcl version of old spike-face
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

# create a cyberware source
#
vtkCyberReader cyber
    cyber SetFileName "../../../data/fran_cut"
vtkDecimate deci
    deci SetInput [cyber GetOutput]
    deci SetTargetReduction 0.90
    deci SetInitialError 0.0002
    deci SetErrorIncrement 0.0002
    deci SetMaximumError 0.001
    deci SetAspectRatio 20
vtkPolyNormals normals
    normals SetInput [deci GetOutput]
vtkStripper stripper
    stripper SetInput [normals GetOutput]
vtkMaskPolyData mask
    mask SetInput [stripper GetOutput]
    mask SetOnRatio 2
vtkPolyMapper cyberMapper
    cyberMapper SetInput [mask GetOutput]
vtkActor cyberActor
    cyberActor SetMapper cyberMapper
eval [cyberActor GetProperty] SetColor 1.0 0.49 0.25

# Add the actors to the renderer, set the background and size
#
$ren1 AddActor cyberActor
$ren1 SetBackground 1 1 1
$renWin SetSize 500 500

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract}
set cam1 [$ren1 GetActiveCamera]
$cam1 Azimuth 120
set sphereProp [cyberActor GetProperty]

# do stereo example
$cam1 Zoom 1.4
$renWin Render

#$renWin SetFileName "stripF2.tcl.ppm"
#$renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


