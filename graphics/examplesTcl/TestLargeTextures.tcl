catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# script that tests rendering of textures that are quite large (not immense, to be fair)
# (these should trigger a resampling down to the maximum (not ideal...) if using OpenGL)

# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# create pipeline
#

vtkPNMReader pnmReader
    pnmReader SetFileName "$VTK_DATA/masonry.ppm"

# resample the image so that it becomes considerably large
vtkImageResample resample
    resample SetInput [pnmReader GetOutput]
    resample SetAxisOutputSpacing 0 0.1
    resample SetAxisOutputSpacing 1 0.1

vtkTexture atext
    atext SetInput [resample GetOutput]
    atext InterpolateOn

# gnerate plane to map texture on to
vtkPlaneSource plane
    plane SetXResolution 1
    plane SetYResolution 1
vtkPolyDataMapper textureMapper
    textureMapper SetInput [plane GetOutput]
vtkActor textureActor
    textureActor SetMapper textureMapper
    textureActor SetTexture atext

# Create the RenderWindow, Renderer
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor textureActor
renWin SetSize 250 250
ren1 SetBackground 0.1 0.2 0.4

# prevent the tk window from showing up then start the event loop
wm withdraw .

iren Initialize

iren SetUserMethod {wm deiconify .vtkInteract}

renWin SetFileName "TestLargeTextures.tcl.ppm"
renWin SaveImageAsPPM



