catch {load vtktcl}
#
source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/colors.tcl
source permutes.tcl

# Now create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderer ren2
vtkRenderer ren3
vtkRenderWindow renWin
  renWin AddRenderer ren1
  renWin AddRenderer ren2
  renWin AddRenderer ren3

vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

set sliceNumber 40
set ROWS 470
set COLUMNS 500
set GREYSTUDY ../../../vtkdata/frog/frog
set SEGMENTSTUDY ../../../vtkdata/frog/frogTissue
set START_SLICE 1
set END_SLICE 138
set PIXEL_SIZE 1
set SPACING 1.5
set centerX [expr ( $COLUMNS / 2 )]
set centerY [expr ( $ROWS / 2 )]
set centerZ [expr ( $END_SLICE - $START_SLICE ) / 2]
set endX [expr ( $COLUMNS - 1 ) ]
set endY [expr ( $ROWS - 1 ) ]
set endZ [expr ( $END_SLICE - 1 ) ]
set originX [expr ( $COLUMNS / 2.0 ) * $PIXEL_SIZE * -1.0]
set originY [expr ( $ROWS / 2.0 ) * $PIXEL_SIZE * -1.0]
set SLICE_ORDER si

vtkPNMReader greyReader
  eval greyReader SetFilePrefix $GREYSTUDY
  eval greyReader SetDataSpacing $PIXEL_SIZE $PIXEL_SIZE $SPACING
  eval greyReader SetImageRange $sliceNumber $sliceNumber
  greyReader DebugOn

vtkImageConstantPad greyPadder
  greyPadder SetInput [greyReader GetOutput]
  greyPadder SetOutputWholeExtent 0 511 0 511 0 0 0 0 
  greyPadder SetConstant 0

vtkPlaneSource greyPlane

vtkTransformPolyDataFilter greyTransform
  greyTransform SetTransform $SLICE_ORDER
  greyTransform SetInput [greyPlane GetOutput]

vtkPolyDataNormals greyNormals
  greyNormals SetInput [greyTransform GetOutput]
  greyNormals FlipNormalsOff

vtkWindowLevelLookupTable wllut
  wllut SetWindow 255
  wllut SetLevel 128
  wllut SetTableRange 0 255
  wllut Build

vtkPolyDataMapper greyMapper
  greyMapper SetInput [greyPlane GetOutput]
  greyMapper ImmediateModeRenderingOn

vtkTexture greyTexture
  greyTexture SetInput [greyPadder GetOutput]
  greyTexture SetLookupTable wllut
  greyTexture MapColorScalarsThroughLookupTableOn
  greyTexture InterpolateOn

vtkActor greyActor
  greyActor SetMapper greyMapper
  greyActor SetTexture greyTexture

vtkPNMReader segmentReader
  eval segmentReader SetFilePrefix $SEGMENTSTUDY
  eval segmentReader SetDataSpacing $PIXEL_SIZE $PIXEL_SIZE $SPACING
  eval segmentReader SetImageRange $sliceNumber $sliceNumber
  segmentReader DebugOn

vtkImageConstantPad segmentPadder
  segmentPadder SetInput [segmentReader GetOutput]
  segmentPadder SetOutputWholeExtent 0 511 0 511 0 0 0 0 
  segmentPadder SetConstant 0

vtkPlaneSource segmentPlane

vtkTransformPolyDataFilter segmentTransform
  segmentTransform SetTransform $SLICE_ORDER
  segmentTransform SetInput [segmentPlane GetOutput]

vtkPolyDataNormals segmentNormals
  segmentNormals SetInput [segmentTransform GetOutput]
  segmentNormals FlipNormalsOn

vtkLookupTable colorlut
    colorlut SetNumberOfColors 17
    colorlut SetTableRange 0 16
    colorlut Build

source frogLut.tcl

vtkPolyDataMapper segmentMapper
    segmentMapper SetInput [segmentPlane GetOutput]
    segmentMapper ImmediateModeRenderingOn

vtkTexture segmentTexture
  segmentTexture SetInput [segmentPadder GetOutput]
  segmentTexture SetLookupTable colorlut
  segmentTexture MapColorScalarsThroughLookupTableOn
  segmentTexture InterpolateOff

vtkActor segmentActor
  segmentActor SetMapper segmentMapper
  segmentActor SetTexture segmentTexture

vtkActor segmentOverlayActor
  segmentOverlayActor SetMapper segmentMapper
  segmentOverlayActor SetTexture segmentTexture

[segmentOverlayActor GetProperty] SetOpacity .5
ren1 SetBackground 0 0 0
ren1 SetViewport 0 .5 .5 1
renWin SetSize 640 480
ren1 AddActor greyActor

ren2 SetBackground 0 0 0
ren2 SetViewport .5 .5 1 1
ren2 AddActor segmentActor

vtkCamera cam1
ren1 SetActiveCamera cam1
ren1 ResetCamera

ren3 AddActor greyActor
ren3 AddActor segmentOverlayActor
segmentOverlayActor SetPosition 0 0 .01

ren3 SetBackground 0 0 0
ren3 SetViewport 0 0 1 .5

ren2 SetActiveCamera [ren1 GetActiveCamera]
ren3 SetActiveCamera [ren1 GetActiveCamera]

source WindowLevel.tcl
WindowLevelOn wllut

renWin Render

iren Initialize;

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract};

# prevent the tk window from showing up then start the event loop
wm withdraw .

proc slice { number } {
   global maxX maxY
   greyReader SetImageRange $number $number
   [greyReader GetOutput] SetOrigin 0 0 0
   segmentReader SetImageRange $number $number
   [segmentReader GetOutput] SetOrigin 0 0 0
   renWin Render
}

proc slices { } {
    global START_SLICE END_SLICE
    for { set i $START_SLICE } { $i <= $END_SLICE } { incr i} {
      slice $i
      update
    }
}
