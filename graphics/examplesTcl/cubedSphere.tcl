# Mix imaging and visualization; warp an image in z-direction
#

catch {load vtktcl}
source ../../examplesTcl/vtkInt.tcl

vtkSphere sphere
    sphere SetCenter 1 1 1
    sphere SetRadius 0.9

vtkSampleFunction sample
    sample SetImplicitFunction sphere
    sample SetModelBounds  0 2 0 2 0 2
    sample SetSampleDimensions 30 30 30
    sample ComputeNormalsOff


vtkThreshold threshold
    threshold SetInput [sample GetOutput]
    threshold ThresholdByLower 0.5

vtkGeometryFilter geometry
  geometry SetInput [threshold GetOutput]

vtkDataSetMapper mapper
  mapper SetInput [geometry GetOutput]

vtkActor actor
  actor SetMapper mapper

# Create renderer stuff
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor
[ren1 GetActiveCamera] Azimuth 20
[ren1 GetActiveCamera] Elevation 30
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 450 450

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.4
iren Initialize

#renWin SetFileName "valid/imageWarp.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


