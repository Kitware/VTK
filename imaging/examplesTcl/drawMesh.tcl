# This example demonstrates how to draw 3D polydata (in world coordinates) in
# the 2D overlay plane. Useful for selection loops, etc.
catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# create the visualization pipeline
#
# create a sphere
vtkSphereSource sphere
    sphere SetThetaResolution 10
    sphere SetPhiResolution 20

# extract a group of triangles and their boundary edges
vtkGeometryFilter gf
    gf SetInput [sphere GetOutput]
    gf CellClippingOn
    gf SetCellMinimum 10
    gf SetCellMaximum 17
vtkFeatureEdges edges
    edges SetInput [gf GetOutput]

# setup the mapper to draw points from world coordinate system
vtkCoordinate worldCoordinates
    worldCoordinates SetCoordinateSystemToWorld
vtkPolyDataMapper2D mapLines
    mapLines SetInput [edges GetOutput]
    mapLines SetTransformCoordinate worldCoordinates
vtkActor2D linesActor
    linesActor SetMapper mapLines
    [linesActor GetProperty] SetColor 0 1 0

# mapper and actor
vtkPolyDataMapper mapper
    mapper SetInput [sphere GetOutput]
vtkActor sphereActor
    sphereActor SetMapper mapper

# Create graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor sphereActor
ren1 AddActor2D linesActor
renWin SetSize 250 250

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 0.294791 17.3744
$cam1 SetFocalPoint 0 0 0
$cam1 SetPosition 1.60648 0.00718286 -2.47173
$cam1 SetViewUp -0.634086 0.655485 -0.410213
$cam1 Zoom 1.25
iren Initialize

renWin SetFileName "drawMesh.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .

