catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

## LOx post CFD case study with anti-aliasing

# get helper scripts
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

# read data
#
vtkPLOT3DReader pl3d
  pl3d SetXYZFileName "$VTK_DATA/postxyz.bin"
  pl3d SetQFileName "$VTK_DATA/postq.bin"
  pl3d Update

# computational planes
vtkStructuredGridGeometryFilter floorComp
  floorComp SetExtent 0 37 0 75 0 0
  floorComp SetInput [pl3d GetOutput]
  floorComp Update
vtkPolyDataMapper floorMapper
  floorMapper SetInput [floorComp GetOutput]
  floorMapper ScalarVisibilityOff
vtkActor floorActor
  floorActor SetMapper floorMapper
  [floorActor GetProperty] SetRepresentationToWireframe
  [floorActor GetProperty] SetColor 0 0 0

vtkStructuredGridGeometryFilter postComp
  postComp SetExtent 10 10 0 75 0 37
  postComp SetInput [pl3d GetOutput]
vtkPolyDataMapper postMapper
  postMapper SetInput [postComp GetOutput]
  postMapper ScalarVisibilityOff
vtkActor postActor
  postActor SetMapper postMapper
  [postActor GetProperty] SetColor 0 0 0
  [postActor GetProperty] SetRepresentationToWireframe

vtkStructuredGridGeometryFilter fanComp
  fanComp SetExtent 0 37 38 38 0 37
  fanComp SetInput [pl3d GetOutput]
vtkPolyDataMapper fanMapper
  fanMapper SetInput [fanComp GetOutput]
vtkActor fanActor
  fanActor SetMapper fanMapper
  [fanActor GetProperty] SetColor 0 0 0
  [fanActor GetProperty] SetRepresentationToWireframe

# outline
vtkStructuredGridOutlineFilter outline
  outline SetInput [pl3d GetOutput]
vtkPolyDataMapper outlineMapper
  outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
  outlineActor SetMapper outlineMapper
set outlineProp [outlineActor GetProperty]
  eval $outlineProp SetColor 0 0 0

# Create graphics stuff
vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor floorActor
ren1 AddActor postActor
ren1 AddActor fanActor

vtkCamera aCam
  aCam SetFocalPoint 0.00657892 0 2.41026
  aCam SetPosition -1.94838 -47.1275 39.4607
  aCam SetViewUp 0.00653193 0.617865 0.786257

ren1 SetBackground 1 1 1
ren1 SetActiveCamera aCam
renWin SetSize 400 400
renWin SetAAFrames 10

iren Initialize
renWin Render

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

renWin Render
#renWin SetFileName "LOxGridAA.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .






