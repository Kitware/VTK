catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Extract fran's nose with a bounding box
# get the interactor ui
source $VTK_TCL/vtkInt.tcl

vtkPNMReader pnm1
    pnm1 SetFileName "$VTK_DATA/fran_cut.ppm"

vtkTexture atext
  atext SetInput [pnm1 GetOutput]
  atext InterpolateOn

# create a cyberware source
#
vtkPolyDataReader cyber
    cyber SetFileName "$VTK_DATA/fran_cut.vtk"
    cyber Update
vtkPlanes planes
    eval planes SetBounds [[cyber GetOutput] GetBounds]
vtkExtractPolyDataGeometry extract
    extract SetInput [cyber GetOutput]
    extract SetImplicitFunction planes
    extract ExtractBoundaryCellsOn
vtkPolyDataMapper mapper
    mapper SetInput [extract GetOutput]
vtkActor fran
    fran SetMapper mapper
#    fran SetTexture atext

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor fran
ren1 SetBackground 1 1 1

renWin SetSize 250 250

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .


