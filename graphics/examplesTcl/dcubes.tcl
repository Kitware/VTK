catch {load vtktcl}
# get the interactor ui
source vtkInt.tcl
source colors.tcl

# Create the render master
vtkRenderMaster rm

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow]
set ren1   [$renWin MakeRenderer]
set iren [$renWin MakeRenderWindowInteractor]

# create pipeline
#
vtkStructuredPointsReader reader
    reader SetFileName "../../data/ironProt.vtk"
#vtkRecursiveDividingCubes iso
vtkDividingCubes iso
    iso SetInput [reader GetOutput]
    iso SetValue 128
    iso SetDistance 0.5
    iso SetIncrement 1
vtkPolyMapper isoMapper
    isoMapper SetInput [iso GetOutput]
    isoMapper ScalarsVisibleOff
vtkActor isoActor
    isoActor SetMapper isoMapper
    eval [isoActor GetProperty] SetColor $bisque

vtkOutlineFilter outline
    outline SetInput [reader GetOutput]
vtkPolyMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    eval [outlineActor GetProperty] SetColor $black

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors outlineActor
$ren1 AddActors isoActor
$ren1 SetBackground 1 1 1
$renWin SetSize 500 500
$ren1 SetBackground 0.1 0.2 0.4

vtkCamera cam1
    cam1 SetClippingRange 19.1589 957.946
    cam1 SetFocalPoint 33.7014 26.706 30.5867
    cam1 SetPosition 150.841 89.374 -107.462
    cam1 CalcViewPlaneNormal
    cam1 SetViewUp -0.190015 0.944614 0.267578
$ren1 SetActiveCamera cam1

$iren Initialize

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract}

#$renWin SetFileName "dcubes.tcl.ppm"
#$renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


