catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }
#
# clip a surface with a plane and a plane with an implicit volume
#

# get the interactor ui
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl
source $VTK_TCL/frog/SliceOrder.tcl

# Create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

vtkPolyDataReader skinReader
  skinReader SetFileName "$VTK_DATA/skin.vtk"

set RESOLUTION 256
set START_SLICE 1
set END_SLICE 93
set PIXEL_SIZE .8
set centerX [expr ( $RESOLUTION / 2 )]
set centerY [expr ( $RESOLUTION / 2 )]
set centerZ [expr ( $END_SLICE - $START_SLICE ) / 2]
set endX [expr ( $RESOLUTION - 1 ) ]
set endY [expr ( $RESOLUTION - 1 ) ]
set endZ [expr ( $END_SLICE - 1 ) ]
set origin [expr ( $RESOLUTION / 2.0 ) * $PIXEL_SIZE * -1.0]
set SLICE_ORDER si
vtkVolume16Reader reader;
  eval reader SetDataDimensions $RESOLUTION $RESOLUTION
  reader SetFilePrefix $VTK_DATA/fullHead/headsq
  eval reader SetDataSpacing $PIXEL_SIZE $PIXEL_SIZE 1.5
  eval reader SetDataOrigin $origin $origin 1.5
  eval reader SetImageRange $START_SLICE $END_SLICE
  reader SetHeaderSize 0
  reader SetDataMask 0x7fff;
  reader SetDataByteOrderToLittleEndian
  reader SetTransform si

vtkExtractVOI aPlaneSection
   aPlaneSection SetVOI $centerX $centerX 0 $endZ 0 $endY
   aPlaneSection SetInput [reader GetOutput]
   aPlaneSection Update

set bounds [[aPlaneSection GetOutput] GetBounds]

vtkPlaneSource aPlaneSource
  aPlaneSource SetOrigin [lindex $bounds 0] [lindex $bounds 2] [lindex $bounds 4]
  aPlaneSource SetPoint1 [lindex $bounds 0] [lindex $bounds 2] [lindex $bounds 5]
  aPlaneSource SetPoint2 [lindex $bounds 0] [lindex $bounds 3] [lindex $bounds 4]
  aPlaneSource SetResolution 200 100

vtkImplicitVolume aVolumeModel
  aVolumeModel SetVolume [reader GetOutput]

vtkClipPolyData aClipper
  aClipper SetInput [aPlaneSource GetOutput]
  aClipper SetClipFunction aVolumeModel
  aClipper SetValue 600.5
  aClipper GenerateClipScalarsOn
  aClipper Update

vtkWindowLevelLookupTable wlLut 
  wlLut SetWindow 2047.0
  wlLut SetLevel 1023.5
  wlLut Build

vtkPolyDataMapper aClipperMapper
  aClipperMapper SetInput [aClipper GetOutput]
  aClipperMapper SetScalarRange 0 2047
  aClipperMapper SetLookupTable wlLut
  aClipperMapper ScalarVisibilityOn

vtkActor cut
  cut SetMapper aClipperMapper

vtkPlane aPlane
  eval aPlane SetOrigin [aPlaneSource GetOrigin]
  eval aPlane SetNormal [aPlaneSource GetNormal]

vtkClipPolyData aCutter
  aCutter SetClipFunction aPlane
  aCutter SetInput [skinReader GetOutput]

vtkPolyDataMapper skinMapper
  skinMapper SetInput [aCutter GetOutput]
  skinMapper ScalarVisibilityOff

vtkActor skin
  skin SetMapper skinMapper
  eval [skin GetProperty] SetDiffuseColor $flesh
  eval [skin GetProperty] SetDiffuse .8
  eval [skin GetProperty] SetSpecular .5
  eval [skin GetProperty] SetSpecularPower 30

vtkProperty backProp
  eval backProp SetDiffuseColor $flesh
  backProp SetDiffuse .2
  backProp SetSpecular .5
  backProp SetSpecularPower 30

skin SetBackfaceProperty backProp

ren1 AddActor skin
ren1 AddActor cut

ren1 SetBackground 1 1 1
renWin SetSize 480 480
[ren1 GetActiveCamera] SetViewUp 0 -1 0;
[ren1 GetActiveCamera] Azimuth 230
[ren1 GetActiveCamera] Elevation 30
[ren1 GetActiveCamera] Dolly 1.2
ren1 ResetCameraClippingRange

iren Initialize;

renWin Render

#renWin SetFileName "implicitVolume.tcl.ppm"
#renWin SaveImageAsPPM

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract};

# prevent the tk window from showing up then start the event loop
wm withdraw .
