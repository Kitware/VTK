catch {load vtktcl}
# Generate implicit model of a sphere
#
source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/colors.tcl

# Create renderer stuff
#
vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

# create pipeline
#
vtkSuperquadric squad
  squad SetToroidal 1
  squad SetThickness 0.444
  squad SetPhiRoundness 0.3
  squad SetThetaRoundness 3
  squad SetSize 4
  squad SetScale 1 1 0.5

vtkTransform ntrans
  ntrans Identity
  ntrans Scale 0.25 0.25 0.25
  ntrans Inverse

squad SetTransform ntrans

vtkSampleFunction sample
  sample SetImplicitFunction squad
  sample SetSampleDimensions 64 64 64
  sample ComputeNormalsOff

vtkContourFilter iso
  iso SetInput [sample GetOutput]
  iso SetValue 1 0

vtkPolyDataMapper isoMapper
  isoMapper SetInput [iso GetOutput]
  isoMapper ScalarVisibilityOn

vtkActor isoActor
  isoActor SetMapper isoMapper
  eval [isoActor GetProperty] SetColor $peacock

vtkOutlineFilter outline
  outline SetInput [sample GetOutput]
vtkPolyDataMapper outlineMapper
  outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
  outlineActor SetMapper outlineMapper
  [outlineActor GetProperty] SetColor 0 0 0

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor isoActor
ren1 SetBackground 1 1 1
renWin SetSize 400 400
[ren1 GetActiveCamera] Zoom 1.5
[ren1 GetActiveCamera] Elevation 40
[ren1 GetActiveCamera] Azimuth -20
iren Initialize

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

#renWin SetFileName "superquad.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


