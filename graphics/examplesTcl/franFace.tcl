catch {load vtktcl}
# this is a tcl version of old franFace
# get the interactor ui
source vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create a cyberware source
#
vtkCyberReader cyber
    cyber SetFileName "../../../data/fran_cut"
vtkPolyDataMapper cyberMapper
    cyberMapper SetInput [cyber GetOutput]

vtkPNMReader pnm1
    pnm1 SetFileName "../../../data/fran_cut.ppm"

vtkTexture atext
  atext SetInput [pnm1 GetOutput]
  atext InterpolateOn

vtkActor cyberActor
  cyberActor SetMapper cyberMapper
  cyberActor SetTexture atext

# Add the actors to the renderer, set the background and size
#
ren1 AddActor cyberActor
ren1 SetBackground 1 1 1
renWin SetSize 500 500

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
[ren1 GetActiveCamera] Azimuth 90
iren Initialize

#renWin SetFileName "franFace.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


