package require vtk

# Test label reading from an MNI tag file

#
# write to the temp directory if possible, otherwise use .
#
set dir "."
if {[info commands "rtTester"] == "rtTester"}  {
   set dir [rtTester GetTempDirectory]
}

# create some random points in a sphere
#
vtkPointSource sphere1
  sphere1 SetNumberOfPoints 13

vtkTransform xform
  xform RotateWXYZ 20 1 0 0

vtkTransformFilter xformFilter
  xformFilter SetTransform xform
  xformFilter SetInputConnection [sphere1 GetOutputPort]

vtkStringArray labels
  labels InsertNextValue "0"
  labels InsertNextValue "1"
  labels InsertNextValue "2"
  labels InsertNextValue "3"
  labels InsertNextValue "Halifax"
  labels InsertNextValue "Toronto"
  labels InsertNextValue "Vancouver"
  labels InsertNextValue "Larry"
  labels InsertNextValue "Bob"
  labels InsertNextValue "Jackie"
  labels InsertNextValue "10"
  labels InsertNextValue "11"
  labels InsertNextValue "12"

vtkDoubleArray weights
  weights InsertNextValue 1.0
  weights InsertNextValue 1.1
  weights InsertNextValue 1.2
  weights InsertNextValue 1.3
  weights InsertNextValue 1.4
  weights InsertNextValue 1.5
  weights InsertNextValue 1.6
  weights InsertNextValue 1.7
  weights InsertNextValue 1.8
  weights InsertNextValue 1.9
  weights InsertNextValue 0.9
  weights InsertNextValue 0.8
  weights InsertNextValue 0.7

set fname "$dir/mni-tagtest.tag"

vtkMNITagPointWriter writer
  writer SetFileName "$fname"
  writer SetInputConnection [sphere1 GetOutputPort]
  writer SetInputConnection 1 [xformFilter GetOutputPort]
  writer SetLabelText labels
  writer SetWeights weights
  writer SetComments "Volume 1: sphere points\nVolume 2: transformed points"
  writer Write

vtkMNITagPointReader reader
  reader CanReadFile "$fname"
  reader SetFileName "$fname"

vtkTextProperty textProp
  textProp SetFontSize 12
  textProp SetColor 1.0 1.0 0.5

vtkPointSetToLabelHierarchy labelHier
  labelHier SetInputConnection [reader GetOutputPort]
  labelHier SetTextProperty textProp
  labelHier SetLabelArrayName "LabelText"
  labelHier SetMaximumDepth 15
  labelHier SetTargetLabelCount 12

vtkLabelPlacementMapper labelMapper
  labelMapper SetInputConnection [labelHier GetOutputPort]
  labelMapper UseDepthBufferOff
  labelMapper SetShapeToRect
  labelMapper SetStyleToOutline

vtkActor2D labelActor
  labelActor SetMapper labelMapper

vtkSphereSource glyphSource
  glyphSource SetRadius 0.01

vtkGlyph3D glyph
  glyph SetSourceConnection [glyphSource GetOutputPort]
  glyph SetInputConnection [reader GetOutputPort]

vtkDataSetMapper mapper
  mapper SetInputConnection [glyph GetOutputPort]

vtkActor actor
  actor SetMapper mapper

# Create rendering stuff
vtkRenderer ren1
vtkRenderWindow renWin
    renWin SetMultiSamples 0
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddViewProp actor
ren1 AddViewProp labelActor
ren1 SetBackground 0 0 0
renWin SetSize 300 300
renWin Render

# render the image
#
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .
