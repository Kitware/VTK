catch {load vtktcl}
# Create a fancy image of a 2D Delaunay triangulation. Points are randomly 
# generated.

# get the interactor ui
source vtkInt.tcl

# get some nice colors
source colors.tcl

# create some points
#
vtkMath math
vtkFloatPoints points
for {set i 0} {$i<50} {incr i 1} {
    eval points InsertPoint $i [math Random 0 1] [math Random 0 1] 0.0
}

vtkPolyData profile
    profile SetPoints points

# triangulate them
#
vtkDelaunay2D del
    del SetInput profile
    del SetTolerance 0.001
vtkPolyMapper mapMesh
    mapMesh SetInput [del GetOutput]
vtkActor meshActor
    meshActor SetMapper mapMesh
    eval [meshActor GetProperty] SetColor .1 .2 .4

vtkExtractEdges extract
    extract SetInput [del GetOutput]
vtkTubeFilter tubes
    tubes SetInput [extract GetOutput]
    tubes SetRadius 0.01
    tubes SetNumberOfSides 6
vtkPolyMapper mapEdges
    mapEdges SetInput [tubes GetOutput]
vtkActor edgeActor
    edgeActor SetMapper mapEdges
eval [edgeActor GetProperty] SetColor $peacock
    [edgeActor GetProperty] SetSpecularColor 1 1 1
    [edgeActor GetProperty] SetSpecular 0.3
    [edgeActor GetProperty] SetSpecularPower 20
    [edgeActor GetProperty] SetAmbient 0.2
    [edgeActor GetProperty] SetDiffuse 0.8

vtkSphereSource ball
    ball SetRadius 0.025
    ball SetThetaResolution 12
    ball SetPhiResolution 12
vtkGlyph3D balls
    balls SetInput [del GetOutput]
    balls SetSource [ball GetOutput]
vtkPolyMapper mapBalls
    mapBalls SetInput [balls GetOutput]
vtkActor ballActor
    ballActor SetMapper mapBalls
    eval [ballActor GetProperty] SetColor $hot_pink
    [ballActor GetProperty] SetSpecularColor 1 1 1
    [ballActor GetProperty] SetSpecular 0.3
    [ballActor GetProperty] SetSpecularPower 20
    [ballActor GetProperty] SetAmbient 0.2
    [ballActor GetProperty] SetDiffuse 0.8

# Create graphics objects
vtkRenderMaster rm

# Now create the rendering window, renderer, and interactive renderer
set renWin [rm MakeRenderWindow]
set ren1   [$renWin MakeRenderer]
set iren   [$renWin MakeRenderWindowInteractor]

# Add the actors to the renderer, set the background and size
$ren1 AddActors ballActor
#$ren1 AddActors meshActor
$ren1 AddActors edgeActor
$ren1 SetBackground 1 1 1
$renWin SetSize 150 150
#$renWin SetSize 500 500

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract}
[$ren1 GetActiveCamera] Zoom 1.5
$iren Initialize

$renWin SetFileName "DelMesh.ppm"
#$renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


