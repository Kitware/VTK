catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkSTLReader sr
    sr SetFileName $VTK_DATA/42400-IDGH.stl

vtkExtractPolyDataPiece piece
    piece SetInput [sr GetOutput]

vtkFeatureEdges edges
    edges SetInput [piece GetOutput]
    edges ColoringOn
    # all the other types of edges are on by default
    edges ManifoldEdgesOn

vtkPolyDataMapper   stlMapper
#    stlMapper SetInput [sr GetOutput]
    stlMapper SetInput [edges GetOutput]
    stlMapper SetNumberOfPieces 2
    stlMapper SetPiece 0
    stlMapper SetScalarRange 0 0.8

vtkLODActor stlActor
    stlActor SetMapper stlMapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor stlActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 500 500

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.4
iren Initialize
#renWin SetFileName "stl.tcl.ppm"
#renWin SaveImageAsPPM

# test regeneration of the LODMappers
stlActor Modified

# prevent the tk window from showing up then start the event loop
wm withdraw .
