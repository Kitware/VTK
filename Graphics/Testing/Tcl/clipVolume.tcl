package require vtk
package require vtkinteraction

# Example demonstrates how to generate a 3D tetrahedra mesh from a volume
#


# Quadric definition
vtkQuadric quadric
  quadric SetCoefficients .5 1 .2 0 .1 0 0 .2 0 0

vtkSampleFunction sample
  sample SetSampleDimensions 20 20 20
  sample SetImplicitFunction quadric
  sample ComputeNormalsOff
    
# Generate tetrahedral mesh
vtkClipVolume clip
  clip SetInputConnection [sample GetOutputPort]
  clip SetValue 1.0
  clip GenerateClippedOutputOff

vtkDataSetMapper clipMapper
  clipMapper SetInputConnection [clip GetOutputPort]
  clipMapper ScalarVisibilityOff

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
