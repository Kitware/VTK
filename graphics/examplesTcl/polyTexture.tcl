catch {load vtktcl}
source ../../examplesTcl/vtkInt.tcl

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Plane source to generate texture
vtkPlaneSource plane
plane SetResolution 63 63; # resolution specifies number of quads

# Transform for texture and quad
vtkTransform aTransform
  aTransform RotateX 30

vtkTransformPolyDataFilter planeTransform
  planeTransform SetTransform aTransform
  planeTransform SetInput [plane GetOutput]

# Generate a synthetic volume from quadric
vtkQuadric quadric
  quadric SetCoefficients .5 1 .2 0 .1 0 0 .2 0 0

vtkTransform transformSamples
  transformSamples RotateX 30
  transformSamples Inverse

vtkSampleFunction sample
  sample SetSampleDimensions 30 30 30
  sample SetImplicitFunction quadric
  sample Update
    
# Probe the synthetic volume
vtkProbeFilter probe
  probe SetInput [planeTransform GetOutput]
  probe SetSource [sample GetOutput]
  probe Update

# Create Structured points and set the scalars
vtkStructuredPoints structuredPoints
  [structuredPoints GetPointData] SetScalars [[[probe GetOutput] GetPointData] GetScalars]
  structuredPoints SetDimensions 64 64 1; # these dimensions must match probe point count

# Define the texture with structured points
vtkTexture polyTexture
  polyTexture SetInput structuredPoints

# The quad we'll see
vtkPlaneSource quad
  quad SetResolution 1 1

# Use the same transform as the probed points
vtkTransformPolyDataFilter quadTransform
  quadTransform SetTransform aTransform
  quadTransform SetInput [quad GetOutput]

vtkPolyDataMapper quadMapper
  quadMapper SetInput [quadTransform GetOutput]

vtkActor quadActor
  quadActor SetMapper quadMapper
  quadActor SetTexture polyTexture

# Create outline
vtkOutlineFilter outline
  outline SetInput [sample GetOutput]

vtkPolyDataMapper outlineMapper
  outlineMapper SetInput [outline GetOutput]

vtkActor outlineActor
  outlineActor SetMapper outlineMapper
  eval [outlineActor GetProperty] SetColor 0 0 0

ren1 SetBackground 1 1 1
ren1 AddActor quadActor
ren1 AddActor outlineActor
[ren1 GetActiveCamera] Dolly 1.3

iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

wm withdraw .

# Update the scalars in the structured points with the probe's output
proc updateStructuredPoints {} {
  [structuredPoints GetPointData] SetScalars [[[probe GetOutput] GetPointData] GetScalars]
}

# Transform the probe and resample
proc resample {} {
  # Transform the probe points and the quad
  aTransform RotateY 10
  # Force an update on the probe since the pipeline is broken
  probe Update
  renWin Render
}

# Set the probes end method to update the scalars in the structured points
probe SetEndMethod {updateStructuredPoints}

#
for {set i 1} {$i <= 36} {incr i} {
  resample
}
