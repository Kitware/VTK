catch {load vtktcl}
# Generate texture coordinates on a "random" sphere.

# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

vtkPolyDataReader cyber
    cyber SetFileName "../../../vtkdata/fran_cut.vtk"

vtkProjectedTexture tmapper
  tmapper SetPosition 0 0.0 1.2
  tmapper SetFocalPoint 0 0 0
  tmapper SetAspectRatio 1.2 0.7 1
  tmapper SetSRange 0.25 1.25
  tmapper SetInput [cyber GetOutput]

vtkDataSetMapper mapper
  mapper SetInput [tmapper GetOutput]

# load in the texture map and assign to actor
#
vtkPNMReader pnmReader
  pnmReader SetFileName "../../../vtkdata/earth.ppm"
vtkTexture atext
  atext SetInput [pnmReader GetOutput]
  atext InterpolateOn
  atext RepeatOn
vtkActor triangulation
triangulation SetMapper mapper

# Create rendering stuff
vtkRenderer ren1
vtkRenderWindow renWin
renWin AddRenderer ren1
vtkRenderWindowInteractor iren
iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#

ren1 AddActor triangulation
ren1 SetBackground 1 1 1
renWin SetSize 500 500

set cam [ren1 GetActiveCamera]
ren1 ResetCamera
$cam Azimuth 130
$cam Dolly 1.3

set Pos [$cam GetPosition]
set FP  [$cam GetFocalPoint]

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

triangulation SetTexture atext
eval tmapper SetPosition $Pos
eval tmapper SetUp   [$cam GetViewUp]
eval tmapper SetFocalPoint $FP

iren Initialize

#ren1 SetStartRenderMethod move

# prevent the tk window from showing up then start the event loop

wm withdraw .

#renWin SetFileName projTex.tcl.ppm
#renWin SsveImageAsPPM

proc move {} {
    set cam [ren1 GetActiveCamera]
    eval tmapper SetPosition [$cam GetPosition]
    eval tmapper SetUp   [$cam GetViewUp]
    eval tmapper SetFocalPoint [$cam GetFocalPoint]
}
    