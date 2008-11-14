package require vtk
package require vtkinteraction

# Example demonstrates how to generate a 3D tetrahedra mesh from a
# volume. This example differs from the previous clipVolume.tcl examples
# in that it uses the slower ordered triangulator and generates clip scalars.


# Quadric definition
vtkQuadric quadric
  quadric SetCoefficients .5 1 .2 0 .1 0 0 .2 0 0

vtkSampleFunction sample
  sample SetSampleDimensions 20 20 20
  sample SetImplicitFunction quadric
  sample ComputeNormalsOff
  sample Update
    
# Program a bandpass filter to clip a range of data. What we do is transform the 
# scalars so that values lying betweeen (minRange,maxRange) are >= 0.0; all 
# others are < 0.0,
vtkImplicitDataSet dataset
  dataset SetDataSet [sample GetOutput]
vtkImplicitWindowFunction window
  window SetImplicitFunction dataset
  window SetWindowRange 0.5 1.0

# Generate tetrahedral mesh
vtkClipVolume clip
  clip SetInputConnection [sample GetOutputPort]
  clip SetClipFunction window
  clip SetValue 0.0
  clip GenerateClippedOutputOff
  clip Mixed3DCellGenerationOff

vtkGeometryFilter gf
#  gf SetInput [clip GetClippedOutput]
  gf SetInputConnection [clip GetOutputPort]

vtkPolyDataMapper clipMapper
  clipMapper SetInputConnection [gf GetOutputPort]
  clipMapper ScalarVisibilityOn
  eval clipMapper SetScalarRange 0 2

vtkActor clipActor
  clipActor SetMapper clipMapper
  [clipActor GetProperty] SetColor .8 .4 .4

# Create outline
vtkOutlineFilter outline
  outline SetInput [clip GetInput]

vtkPolyDataMapper outlineMapper
  outlineMapper SetInputConnection [outline GetOutputPort]

vtkActor outlineActor
  outlineActor SetMapper outlineMapper
  eval [outlineActor GetProperty] SetColor 0 0 0

# Define graphics objects
vtkRenderer ren1
vtkRenderWindow renWin
    renWin SetMultiSamples 0
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 SetBackground 1 1 1
ren1 AddActor clipActor
ren1 AddActor outlineActor

iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize


wm withdraw .
