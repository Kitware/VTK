# Mix imaging and visualization; warp an image in z-direction
#

catch {load vtktcl}
source ../../examplesTcl/vtkInt.tcl

# read in some structured points
#
vtkPNMReader reader
  reader SetFileName ../../../vtkdata/masonry.ppm

vtkImageGaussianSmooth smooth
  smooth SetInput [reader GetOutput]
  smooth SetStandardDeviation 5 5

vtkStructuredPointsGeometryFilter geometry
  geometry SetInput [reader GetOutput]

vtkWarpScalar warp
  warp SetInput [geometry GetOutput]
  warp SetScaleFactor -0.1

vtkDataSetMapper mapper
  mapper SetInput [warp GetOutput]
  mapper SetScalarRange 0 255
  mapper ImmediateModeRenderingOff

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

# prevent the tk window from showing up then start the event loop
wm withdraw .


