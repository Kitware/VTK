catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# Create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkBMPReader imageIn
  imageIn SetFileName "$VTK_DATA/beach.bmp"
  imageIn ReleaseDataFlagOff
  imageIn Update

vtkBMPReader imageIn2
  imageIn2 SetFileName "$VTK_DATA/beach.bmp"
  imageIn2 ReleaseDataFlagOff
  imageIn2 Update

proc PowerOfTwo {amt} {
  set pow 0
  incr amt -1
  while {1} {
	set amt [expr $amt >> 1]
	incr pow
	if {$amt <= 0} {return [expr 1 << $pow];}
  }
}

set orgX [expr [lindex [[imageIn GetOutput] GetWholeExtent] 1] - [lindex [[imageIn GetOutput] GetWholeExtent] 0] + 1]
set orgY [expr [lindex [[imageIn GetOutput] GetWholeExtent] 3] - [lindex [[imageIn GetOutput] GetWholeExtent] 2] + 1]
set padX [PowerOfTwo $orgX]
set padY [PowerOfTwo $orgY]

vtkImageConstantPad imagePowerOf2
  imagePowerOf2 SetInput [imageIn GetOutput]
  imagePowerOf2 SetOutputWholeExtent 0 [expr $padX - 1] 0 [expr $padY - 1] 0 0

vtkImageRGBToHSV toHSV
  toHSV SetInput [imageIn GetOutput]
  toHSV ReleaseDataFlagOff

vtkImageExtractComponents extractImage
  extractImage SetInput [toHSV GetOutput]
  extractImage SetComponents 2
  extractImage ReleaseDataFlagOff

vtkImageThreshold threshold
  threshold SetInput [extractImage GetOutput]
  threshold ThresholdByUpper 230
  threshold SetInValue 255
  threshold SetOutValue 0
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

vtkImageGaussianSmooth smooth
  smooth SetDimensionality 2
  smooth SetStandardDeviation 1 1
  smooth SetInput [connect GetOutput]

vtkImageShrink3D shrink
  shrink SetInput [smooth GetOutput]
  shrink SetShrinkFactors 2 2 1
  shrink AveragingOn

vtkImageToStructuredPoints toStructuredPoints
  toStructuredPoints SetInput [shrink GetOutput]

vtkStructuredPointsGeometryFilter geometry
  geometry SetInput [toStructuredPoints GetOutput]

vtkTextureMapToPlane geometryTexture
  geometryTexture SetInput [geometry GetOutput]
  geometryTexture SetOrigin 0 0 0
  geometryTexture SetPoint1 [expr $padX - 1] 0 0
  geometryTexture SetPoint2 0 [expr $padY - 1] 0

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
  map SetInput [texturePD GetPolyDataOutput]
  map ScalarVisibilityOff

vtkTexture imageTexture
  imageTexture InterpolateOn
  imageTexture SetInput [imagePowerOf2 GetOutput]

vtkActor clipart
  clipart SetMapper map

ren1 AddActor clipart
[clipart GetProperty] SetDiffuseColor 1 1 1
[clipart GetProperty] SetSpecular .5
[clipart GetProperty] SetSpecularPower 30
[clipart GetProperty] SetDiffuse .9

set camera [ren1 GetActiveCamera]
$camera Azimuth 30
$camera Elevation -30
$camera Dolly 1.5
ren1 ResetCameraClippingRange

ren1 SetBackground 0.2 0.3 0.4
renWin SetSize 320 256
iren Initialize


# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .

vtkImageMaskBits mask
mask SetOperationToXor
mask SetInput [imagePowerOf2 GetOutput]
mask SetEndMethod switchit

vtkImageDataStreamer imageStreamer
imageStreamer SetInput [mask GetOutput]
[imageStreamer GetExtentTranslator] SetSplitModeToXSlab
imageStreamer SetNumberOfStreamDivisions 5

set current 0
proc switchit { } {
   global current 
   
   if { $current } {
      mask SetOperationToXor
      set current 0
   } else {
      mask SetOperationToAnd
      set current 1
   }
}


imageTexture SetInput [imageStreamer GetOutput]

clipart SetTexture imageTexture
renWin Render

## Uncomment this out to cause continual pipeline updates
#while {1} {
#   renWin Render
#   update
#}

# save the regression image
vtkWindowToImageFilter windowToImage
windowToImage SetInput renWin

vtkTIFFWriter tiffimage
tiffimage SetInput [windowToImage GetOutput]
tiffimage SetFileName TestIncrementalImageStreamer1.tcl.tif
#tiffimage Write


 
