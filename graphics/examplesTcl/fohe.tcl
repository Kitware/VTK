catch {load vtktcl}
# this is a tcl version of motor visualization
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
vtkBYUReader byu
    byu SetGeometryFileName "../../../data/fohe.g"
vtkPolyNormals normals
    normals SetInput [byu GetOutput]
vtkPolyMapper byuMapper
    byuMapper SetInput [normals GetOutput]
vtkActor byuActor
    byuActor SetMapper byuMapper

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors byuActor
$ren1 SetBackground 0.2 0.3 0.4
$renWin SetSize 500 500

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract}
$iren Initialize

#$renWin SetFileName "fohe.tcl.ppm"
#$renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


