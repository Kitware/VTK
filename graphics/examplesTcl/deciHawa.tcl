catch {load vtktcl}
catch {load vtktcl}
# decimate hawaii dataset
#
# get the interactor ui
source vtkInt.tcl
source "colors.tcl"
#
vtkRenderMaster rm

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow]
set ren1   [$renWin MakeRenderer]
set iren [$renWin MakeRenderWindowInteractor]

# create a cyberware source
#
vtkPolyReader reader
    reader SetFileName "../../../data/honolulu.vtk"
vtkDecimate deci; 
    deci SetInput [reader GetOutput]
    deci SetTargetReduction 0.9
    deci SetAspectRatio 20
    deci SetInitialError 0.0002
    deci SetErrorIncrement 0.0005
    deci SetMaximumIterations 6
    deci SetInitialFeatureAngle 45
vtkPolyMapper hawaiiMapper
    hawaiiMapper SetInput [deci GetOutput]
vtkActor hawaiiActor
    hawaiiActor SetMapper hawaiiMapper
    eval [hawaiiActor GetProperty] SetColor $turquoise_blue
    eval [hawaiiActor GetProperty] SetWireframe

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors hawaiiActor
$ren1 SetBackground 1 1 1
$renWin SetSize 500 500

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract}

$iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .


