
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

image SetStartMethod "puts -nonewline \"Read...\";flush stdout"
image SetEndMethod "puts \"Complete\""

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
  imagePowerOf2 SetStartMethod "puts -nonewline \"PowerOf2...\";flush stdout"
  imagePowerOf2 SetEndMethod "puts \"Complete\""

vtkImageRGBToHSV toHSV
  toHSV SetInput [image GetOutput]
  toHSV ReleaseDataFlagOff
  toHSV SetNumberOfThreads 1
  toHSV SetStartMethod "puts -nonewline \"toHSV...\";flush stdout"
  toHSV SetEndMethod "puts \"Complete\""

vtkImageExtractComponents extractImage
  extractImage SetInput [toHSV GetOutput]
  extractImage SetComponents 2
  extractImage ReleaseDataFlagOff
  extractImage SetNumberOfThreads 1
  extractImage SetStartMethod "puts -nonewline \"extractV...\";flush stdout"
  extractImage SetEndMethod "puts \"Complete\""

vtkImageThreshold threshold
  threshold SetInput [extractImage GetOutput]
  threshold ThresholdByUpper 230
  threshold SetInValue 255
  threshold SetOutValue 0
  threshold SetNumberOfThreads 1
  threshold Update

  threshold SetStartMethod "puts -nonewline \"Threshold...\";flush stdout"
  threshold SetEndMethod "puts \"Complete\""

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
  connect SetStartMethod "puts -nonewline \"Connectivity...\";flush stdout"
  connect SetEndMethod "puts \"Complete\""
  connect SetNumberOfThreads 1

vtkImageGaussianSmooth smooth
  smooth SetDimensionality 2
  smooth SetStandardDeviation 1 1
  smooth SetInput [connect GetOutput]
  smooth SetNumberOfThreads 1
  smooth SetStartMethod "puts -nonewline \"Smooth...\";flush stdout"
  smooth SetEndMethod "puts \"Complete\""

vtkImageShrink3D shrink
  shrink SetInput [smooth GetOutput]
  shrink SetShrinkFactors 2 2 1
  shrink AveragingOn
  shrink SetStartMethod "puts -nonewline \"Shrink...\";flush stdout"
  shrink SetEndMethod "puts \"Complete\""
  shrink SetNumberOfThreads 1

vtkImageToStructuredPoints toStructuredPoints
  toStructuredPoints SetInput [shrink GetOutput]

vtkStructuredPointsGeometryFilter geometry
  geometry SetInput [toStructuredPoints GetOutput]
  geometry SetStartMethod "puts -nonewline \"Geometry...\";flush stdout"
  geometry SetEndMethod "puts \"Complete\""

vtkTextureMapToPlane geometryTexture
  geometryTexture SetInput [geometry GetOutput]
  geometryTexture SetStartMethod "puts -nonewline \"GeometryTexture...\";flush stdout"
  geometryTexture SetEndMethod "puts \"Complete\""

vtkCastToConcrete geometryPD
  geometryPD SetInput [geometryTexture GetOutput]

vtkClipPolyData clip
  clip SetInput [geometryPD GetPolyDataOutput]
  clip SetValue 5.5
  clip GenerateClipScalarsOff
  clip InsideOutOff
  clip InsideOutOn
  [[clip GetOutput] GetPointData] CopyScalarsOff
  clip SetStartMethod "puts -nonewline \"Clip...\";flush stdout"
  clip SetEndMethod "puts \"Complete\""
  clip Update

vtkTriangleFilter triangles
  triangles SetInput [clip GetOutput]
  triangles SetStartMethod "puts -nonewline \"Triangles...\";flush stdout"
  triangles SetEndMethod "puts \"Complete\""

vtkDecimatePro decimate
  decimate SetInput [triangles GetOutput]
  decimate BoundaryVertexDeletionOn
  decimate SetDegree 25
  decimate PreserveTopologyOn
  decimate SetStartMethod "puts -nonewline \"Decimate...\";flush stdout"
  decimate SetEndMethod "puts \"Complete\""

vtkLinearExtrusionFilter extrude
  extrude SetInput [decimate GetOutput]
  extrude SetExtrusionType 2
  extrude SetScaleFactor -20
  extrude SetStartMethod "puts -nonewline \"Extrude...\";flush stdout"
  extrude SetEndMethod "puts \"Complete\""

vtkPolyDataNormals normals
  normals SetInput [extrude GetOutput]
  normals SetFeatureAngle 80
  normals SetStartMethod "puts -nonewline \"Normals...\";flush stdout"
  normals SetEndMethod "puts \"Complete\""

vtkTransformTextureCoords scaleTexture
  scaleTexture SetInput [normals GetOutput]
  scaleTexture SetScale [expr ($orgX * 1.0) / $padX] [expr ($orgY * 1.0) / $padY] 1.0
  scaleTexture SetOrigin 0 0 0
  scaleTexture SetStartMethod "puts -nonewline \"ScaleTexture...\";flush stdout"
  scaleTexture SetEndMethod "puts \"Complete\""

vtkCastToConcrete texturePD
  texturePD SetInput [scaleTexture GetOutput]

vtkStripper strip
  strip SetInput [texturePD GetPolyDataOutput]
  strip SetStartMethod "puts -nonewline \"Strip...\";flush stdout"
  strip SetEndMethod "puts \"Complete\""

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


