
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# Create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkBMPReader image
  image SetFileName "../../../vtkdata/beach.bmp"
  image ReleaseDataFlagOff
  image Update

proc PowerOfTwo {amt} {
  set pow 0
  incr amt -1
  while {1} {
	set amt [expr $amt >> 1]
	incr pow
	if {$amt <= 0} {return [expr 1 << $pow];}
  }
}

set orgX [expr [lindex [[image GetOutput] GetWholeExtent] 1] - [lindex [[image GetOutput] GetWholeExtent] 0] + 1]
set orgY [expr [lindex [[image GetOutput] GetWholeExtent] 3] - [lindex [[image GetOutput] GetWholeExtent] 2] + 1]
set padX [PowerOfTwo $orgX]
set padY [PowerOfTwo $orgY]

vtkImageConstantPad imagePowerOf2
  imagePowerOf2 SetInput [image GetOutput]
  imagePowerOf2 SetOutputWholeExtent 0 [expr $padX - 1] 0 [expr $padY - 1] 0 0
  imagePowerOf2 SetNumberOfThreads 1

vtkImageRGBToHSV toHSV
  toHSV SetInput [image GetOutput]
  toHSV ReleaseDataFlagOff
  toHSV SetNumberOfThreads 1

vtkImageExtractComponents extractImage
  extractImage SetInput [toHSV GetOutput]
  extractImage SetComponents 2
  extractImage ReleaseDataFlagOff
  extractImage SetNumberOfThreads 1

vtkImageThreshold threshold
  threshold SetInput [extractImage GetOutput]
  threshold ThresholdByUpper 230
  threshold SetInValue 255
  threshold SetOutValue 0
  threshold SetNumberOfThreads 1
  threshold Update


set extent [[threshold GetOutput] GetWholeExtent]
set seed1 "[lindex $extent 0] [lindex $extent 2]"
set seed2 "[lindex $extent 1] [lindex $extent 2]"
set seed3 "[lindex $extent 1] [lindex $extent 3]"
set seed4 "[lindex $extent 0] [lindex $extent 3]"

vtkImageSeedConnectivity connect
  connect SetInput [threshold GetOutput]
  connect SetInputConnectValue 255
  connect SetOutputConnectedValue 255
  connect SetOutputUnconnectedValue 0
  eval connect AddSeed $seed1
  eval connect AddSeed $seed2
  eval connect AddSeed $seed3
  eval connect AddSeed $seed4
  connect SetNumberOfThreads 1

vtkImageGaussianSmooth smooth
  smooth SetDimensionality 2
  smooth SetStandardDeviation 1 1
  smooth SetInput [connect GetOutput]
  smooth SetNumberOfThreads 1

vtkImageShrink3D shrink
  shrink SetInput [smooth GetOutput]
  shrink SetShrinkFactors 2 2 1
  shrink AveragingOn
  shrink SetNumberOfThreads 1

vtkImageToStructuredPoints toStructuredPoints
  toStructuredPoints SetInput [shrink GetOutput]

vtkStructuredPointsGeometryFilter geometry
  geometry SetInput [toStructuredPoints GetOutput]

vtkTextureMapToPlane geometryTexture
  geometryTexture SetInput [geometry GetOutput]

vtkCastToConcrete geometryPD
  geometryPD SetInput [geometryTexture GetOutput]

vtkClipPolyData clip
  clip SetInput [geometryPD GetPolyDataOutput]
  clip SetValue 5.5
  clip GenerateClipScalarsOff
  clip InsideOutOff
  clip InsideOutOn
  [[clip GetOutput] GetPointData] CopyScalarsOff
  clip Update

vtkTriangleFilter triangles
  triangles SetInput [clip GetOutput]

vtkDecimatePro decimate
  decimate SetInput [triangles GetOutput]
  decimate BoundaryVertexDeletionOn
  decimate SetDegree 25
  decimate PreserveTopologyOn

vtkLinearExtrusionFilter extrude
  extrude SetInput [decimate GetOutput]
  extrude SetExtrusionType 2
  extrude SetScaleFactor -20

vtkPolyDataNormals normals
  normals SetInput [extrude GetOutput]
  normals SetFeatureAngle 80

vtkTransformTextureCoords scaleTexture
  scaleTexture SetInput [normals GetOutput]
  scaleTexture SetScale [expr ($orgX * 1.0) / $padX] [expr ($orgY * 1.0) / $padY] 1.0
  scaleTexture SetOrigin 0 0 0

vtkCastToConcrete texturePD
  texturePD SetInput [scaleTexture GetOutput]

vtkStripper strip
  strip SetInput [texturePD GetPolyDataOutput]

vtkPolyDataMapper map
  map SetInput [strip GetOutput]
  map ScalarVisibilityOff

vtkTexture imageTexture
  imageTexture InterpolateOn
  imageTexture SetInput [imagePowerOf2 GetOutput]

vtkActor clipart
  clipart SetMapper map
  clipart SetTexture imageTexture

ren1 AddActor clipart
[clipart GetProperty] SetDiffuseColor 1 1 1
[clipart GetProperty] SetSpecular .5
[clipart GetProperty] SetSpecularPower 30
[clipart GetProperty] SetDiffuse .9

set camera [ren1 GetActiveCamera]
$camera Azimuth 30
$camera Elevation -30
$camera Dolly 1.5

ren1 SetBackground 0.2 0.3 0.4
renWin SetSize 320 256
iren Initialize

renWin Render

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .


