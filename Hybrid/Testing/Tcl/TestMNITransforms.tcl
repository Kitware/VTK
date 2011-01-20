package require vtk

#
# write to the temp directory if possible, otherwise use .
#
set dir "."
if {[info commands "rtTester"] == "rtTester"}  {
   set dir [rtTester GetTempDirectory]
}

# first, create an image to warp
vtkImageGridSource imageGrid
imageGrid SetGridSpacing 16 16 0
imageGrid SetGridOrigin 0 0 0
imageGrid SetDataExtent 0 255 0 255 0 0
imageGrid SetDataScalarTypeToUnsignedChar

vtkLookupTable table
table SetTableRange 0 1
table SetValueRange 1.0 0.0
table SetSaturationRange 0.0 0.0
table SetHueRange 0.0 0.0
table SetAlphaRange 0.0 1.0
table Build

vtkImageMapToColors alpha
alpha SetInputConnection [imageGrid GetOutputPort]
alpha SetLookupTable table

vtkBMPReader reader1
reader1 SetFileName "$VTK_DATA_ROOT/Data/masonry.bmp"

vtkImageBlend blend
blend AddInput [reader1 GetOutput]
blend AddInput [alpha GetOutput]

# next, create a ThinPlateSpline transform

vtkPoints p1
p1 SetNumberOfPoints 8
p1 SetPoint 0 0 0 0
p1 SetPoint 1 0 255 0
p1 SetPoint 2 255 0 0
p1 SetPoint 3 255 255 0
p1 SetPoint 4 96 96 0
p1 SetPoint 5 96 159 0
p1 SetPoint 6 159 159 0
p1 SetPoint 7 159 96 0

vtkPoints p2
p2 SetNumberOfPoints 8
p2 SetPoint 0 0 0 0
p2 SetPoint 1 0 255 0
p2 SetPoint 2 255 0 0
p2 SetPoint 3 255 255 0
p2 SetPoint 4 96 159 0
p2 SetPoint 5 159 159 0
p2 SetPoint 6 159 96 0
p2 SetPoint 7 96 96 0

vtkThinPlateSplineTransform thinPlate0
  thinPlate0 SetSourceLandmarks p1
  thinPlate0 SetTargetLandmarks p2
  thinPlate0 SetBasisToR2LogR

set filename "$dir/mni-thinplatespline.xfm"

# write the tps to a file
vtkMNITransformWriter tpsWriter
  tpsWriter SetFileName "$filename"
  tpsWriter SetTransform thinPlate0
  tpsWriter Write

# read it back
vtkMNITransformReader tpsReader
if { [tpsReader CanReadFile "$filename"] != 0 } {
  tpsReader SetFileName "$filename"
}
  set thinPlate [tpsReader GetTransform]

# make a linear transform
vtkTransform linearTransform
  linearTransform PostMultiply
  linearTransform Translate -127.5 -127.5 0
  linearTransform RotateZ 30
  linearTransform Translate +127.5 +127.5 0

# remove the linear part of the thin plate
vtkGeneralTransform tpsGeneral
  tpsGeneral SetInput $thinPlate
  tpsGeneral PreMultiply
  tpsGeneral Concatenate [[linearTransform GetInverse] GetMatrix]

# convert the thin plate spline into a grid
vtkTransformToGrid transformToGrid
  transformToGrid SetInput tpsGeneral
  transformToGrid SetGridSpacing 16 16 1
  transformToGrid SetGridOrigin -64.5 -64.5 0
  transformToGrid SetGridExtent 0 24 0 24 0 0

vtkGridTransform gridTransform
  gridTransform SetDisplacementGrid [transformToGrid GetOutput]
  gridTransform SetInterpolationModeToCubic

# add back the linear part
vtkGeneralTransform gridGeneral
  gridGeneral SetInput gridTransform
  gridGeneral PreMultiply
  gridGeneral Concatenate [linearTransform GetMatrix]
  # invert for reslice
  gridGeneral Inverse

# write to a file
vtkMNITransformWriter gridWriter
  gridWriter SetFileName "$dir/mni-grid.xfm"
  gridWriter SetComments "TestMNITransforms output transform"
  gridWriter SetTransform gridGeneral
  gridWriter Write

# read it back
vtkMNITransformReader gridReader
  gridReader SetFileName "$dir/mni-grid.xfm"
  set transform [gridReader GetTransform]

# apply the grid warp to the image
vtkImageReslice reslice
  reslice SetInputConnection [blend GetOutputPort]
  reslice SetResliceTransform $transform
  reslice SetInterpolationModeToLinear

# set the window/level to 255.0/127.5 to view full range
vtkImageViewer viewer
viewer SetInputConnection [reslice GetOutputPort]
viewer SetColorWindow 255.0
viewer SetColorLevel 127.5
viewer SetZSlice 0
viewer Render
