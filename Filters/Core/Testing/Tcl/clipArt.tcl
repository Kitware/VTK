package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkTIFFReader imageIn
  imageIn SetFileName "$VTK_DATA_ROOT/Data/beach.tif"

# "beach.tif" image contains ORIENTATION tag which is
# ORIENTATION_TOPLEFT (row 0 top, col 0 lhs) type. The TIFF
# reader parses this tag and sets the internal TIFF image
# orientation accordingly.  To overwrite this orientation with a vtk
# convention of ORIENTATION_BOTLEFT (row 0 bottom, col 0 lhs ), invoke
# SetOrientationType method with parameter value of 4.
  imageIn SetOrientationType 4
  [imageIn GetExecutive] SetReleaseDataFlag 0 0
  imageIn Update

proc PowerOfTwo {amt} {
  set pow 0
  incr amt -1
  while {1} {
	set amt [expr $amt >> 1]
	incr pow
	if {$amt <= 0} {return [expr 1 << $pow];}
  }
}

set orgX [expr [lindex [[imageIn GetExecutive] GetWholeExtent [imageIn GetOutputInformation 0]] 1] - [lindex [[imageIn GetExecutive] GetWholeExtent [imageIn GetOutputInformation 0]] 0] + 1]
set orgY [expr [lindex [[imageIn GetExecutive] GetWholeExtent [imageIn GetOutputInformation 0]] 3] - [lindex [[imageIn GetExecutive] GetWholeExtent [imageIn GetOutputInformation 0]] 2] + 1]
set padX [PowerOfTwo $orgX]
set padY [PowerOfTwo $orgY]

vtkImageConstantPad imagePowerOf2
  imagePowerOf2 SetInputConnection [imageIn GetOutputPort]
  imagePowerOf2 SetOutputWholeExtent 0 [expr $padX - 1] 0 [expr $padY - 1] 0 0

vtkImageRGBToHSV toHSV
  toHSV SetInputConnection [imageIn GetOutputPort]
  [toHSV GetExecutive] SetReleaseDataFlag 0 0

vtkImageExtractComponents extractImage
  extractImage SetInputConnection [toHSV GetOutputPort]
  extractImage SetComponents 2
  [extractImage GetExecutive] SetReleaseDataFlag 0 0

vtkImageThreshold threshold1
  threshold1 SetInputConnection [extractImage GetOutputPort]
  threshold1 ThresholdByUpper 230
  threshold1 SetInValue 255
  threshold1 SetOutValue 0
  threshold1 Update


set extent [[threshold1 GetExecutive] GetWholeExtent [threshold1 GetOutputInformation 0]]

vtkImageSeedConnectivity connect
  connect SetInputConnection [threshold1 GetOutputPort]
  connect SetInputConnectValue 255
  connect SetOutputConnectedValue 255
  connect SetOutputUnconnectedValue 0
  connect AddSeed [lindex $extent 0] [lindex $extent 2]
  connect AddSeed [lindex $extent 1] [lindex $extent 2]
  connect AddSeed [lindex $extent 1] [lindex $extent 3]
  connect AddSeed [lindex $extent 0] [lindex $extent 3]

vtkImageGaussianSmooth smooth
  smooth SetDimensionality 2
  smooth SetStandardDeviation 1 1
  smooth SetInputConnection [connect GetOutputPort]

vtkImageShrink3D shrink
  shrink SetInputConnection [smooth GetOutputPort]
  shrink SetShrinkFactors 2 2 1
  shrink AveragingOn

vtkImageDataGeometryFilter geometry
  geometry SetInputConnection [shrink GetOutputPort]

vtkTextureMapToPlane geometryTexture
  geometryTexture SetInputConnection [geometry GetOutputPort]
  geometryTexture SetOrigin 0 0 0
  geometryTexture SetPoint1 [expr $padX - 1] 0 0
  geometryTexture SetPoint2 0 [expr $padY - 1] 0

vtkCastToConcrete geometryPD
  geometryPD SetInputConnection [geometryTexture GetOutputPort]
  geometryPD Update

vtkClipPolyData clip
  clip SetInputData [geometryPD GetPolyDataOutput]
  clip SetValue 5.5
  clip GenerateClipScalarsOff
  clip InsideOutOff
  clip InsideOutOn
  [[clip GetOutput] GetPointData] CopyScalarsOff
  clip Update

vtkTriangleFilter triangles
  triangles SetInputConnection [clip GetOutputPort]

vtkDecimatePro decimate
  decimate SetInputConnection [triangles GetOutputPort]
  decimate BoundaryVertexDeletionOn
  decimate SetDegree 25
  decimate PreserveTopologyOn

vtkLinearExtrusionFilter extrude
  extrude SetInputConnection [decimate GetOutputPort]
  extrude SetExtrusionType 2
  extrude SetScaleFactor -20

vtkPolyDataNormals normals
  normals SetInputConnection [extrude GetOutputPort]
  normals SetFeatureAngle 80

vtkStripper strip
  strip SetInputConnection [extrude GetOutputPort]

vtkPolyDataMapper map
  map SetInputConnection [strip GetOutputPort]
  map SetInputConnection [normals GetOutputPort]
  map ScalarVisibilityOff

vtkTexture imageTexture
  imageTexture InterpolateOn
  imageTexture SetInputConnection [imagePowerOf2 GetOutputPort]

vtkActor clipart
  clipart SetMapper map
  clipart SetTexture imageTexture

ren1 AddActor clipart
[clipart GetProperty] SetDiffuseColor 1 1 1
[clipart GetProperty] SetSpecular .5
[clipart GetProperty] SetSpecularPower 30
[clipart GetProperty] SetDiffuse .9

ren1 ResetCamera
set camera [ren1 GetActiveCamera]
$camera Azimuth 30
$camera Elevation -30
$camera Dolly 1.5
ren1 ResetCameraClippingRange

ren1 SetBackground 0.2 0.3 0.4
renWin SetSize 320 256
iren Initialize

renWin Render

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .


