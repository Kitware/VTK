catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# this is a tcl version of old spike-face
# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create a cyberware source
#
vtkPolyDataReader cyber
    cyber SetFileName "$VTK_DATA/fran_cut.vtk"
vtkPolyDataNormals normals
    normals SetInput [cyber GetOutput]
vtkPolyDataMapper cyberMapper
    cyberMapper SetInput [normals GetOutput]
vtkActor cyberActor
    cyberActor SetMapper cyberMapper
eval [cyberActor GetProperty] SetColor 1.0 0.49 0.25

# create the spikes using a cone source and a subset of cyber points
#
vtkMaskPoints ptMask
    ptMask SetInput [normals GetOutput]
    ptMask SetOnRatio 100
    ptMask RandomModeOn
vtkConeSource cone
    cone SetResolution 6
vtkTransform transform
    transform Translate 0.5 0.0 0.0
vtkTransformPolyDataFilter transformF
    transformF SetInput [cone GetOutput]
    transformF SetTransform transform
vtkGlyph3D glyph
    glyph SetInput [ptMask GetOutput]
    glyph SetSource [transformF GetOutput]
    glyph SetVectorModeToUseNormal
    glyph SetScaleModeToScaleByVector
    glyph SetScaleFactor 0.004
vtkPolyDataMapper spikeMapper
    spikeMapper SetInput [glyph GetOutput]
vtkActor spikeActor
    spikeActor SetMapper spikeMapper
eval [spikeActor GetProperty] SetColor 0.0 0.79 0.34

# Add the actors to the renderer, set the background and size
#
ren1 AddActor cyberActor
ren1 AddActor spikeActor
ren1 SetBackground 1 1 1
renWin SetSize 500 500
#renWin SetSize 1000 1000
ren1 SetBackground 0.1 0.2 0.4

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
set cam1 [ren1 GetActiveCamera]
set sphereProp [cyberActor GetProperty]
set spikeProp [spikeActor GetProperty]

# do stereo example
$cam1 Zoom 1.4
$cam1 Azimuth 110
renWin Render
renWin SetFileName "valid/spikeF.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


