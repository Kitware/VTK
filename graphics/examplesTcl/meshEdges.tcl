catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Demonstrate how to control z-buffer to reconcile coincident primtives.
# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# read the mesh
#
vtkUnstructuredGridReader mesh
    mesh SetFileName "$VTK_DATA/blow.vtk"
vtkGeometryFilter gf
    gf SetInput [mesh GetOutput]
vtkPolyDataMapper meshMapper
    meshMapper SetInput [gf GetOutput]
    meshMapper ScalarVisibilityOff
    meshMapper GlobalResolveCoincidentPrimitivesOn
vtkActor meshActor
    meshActor SetMapper meshMapper

vtkFeatureEdges edges
    edges SetInput [gf GetOutput]
    edges BoundaryEdgesOn
    edges ManifoldEdgesOn
vtkPolyDataMapper edgeMapper
    edgeMapper SetInput [edges GetOutput]
    edgeMapper ScalarVisibilityOff
vtkActor edgeActor
    edgeActor SetMapper edgeMapper
    [edgeActor GetProperty] SetColor 0 0 0

vtkMaskPoints verts
    verts SetInput [gf GetOutput]
    verts GenerateVerticesOn
    verts SetOnRatio 1
vtkPolyDataMapper vertMapper
    vertMapper SetInput [verts GetOutput]
    vertMapper ScalarVisibilityOff
vtkActor vertActor
    vertActor SetMapper vertMapper
    [vertActor GetProperty] SetColor 1 0 0
    [vertActor GetProperty] SetPointSize 3

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor meshActor
ren1 AddActor edgeActor
ren1 AddActor vertActor
ren1 SetBackground 1 1 1
renWin SetSize 500 500

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
[ren1 GetActiveCamera] Azimuth 90
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .


