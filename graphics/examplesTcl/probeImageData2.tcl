catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

vtkWindowLevelLookupTable lut
  lut SetWindow 2000
  lut SetLevel 1000

# create pipeline
#

vtkImageReader reader1
  reader1 SetDataByteOrderToLittleEndian
  reader1 SetDataExtent 0 255 0 255 1 93
  reader1 SetDataVOI 20 200 20 200 40 40
  reader1 SetFilePrefix "$VTK_DATA/fullHead/headsq"
  reader1 SetDataMask 0x7fff
  reader1 Update

vtkPlaneSource probePlane1
  probePlane1 SetOrigin 10 10 39
  probePlane1 SetPoint1 210 10 39
  probePlane1 SetPoint2 10 210 39
  probePlane1 SetResolution 80 80

vtkProbeFilter probe1
  probe1 SetInput [probePlane1 GetOutput]
  probe1 SetSource [reader1 GetOutput]
 
vtkPolyDataMapper probeMapper1
  probeMapper1 SetInput [probe1 GetOutput]
  eval probeMapper1 SetScalarRange 0 1200

vtkActor probeActor1
  probeActor1 SetMapper probeMapper1
  [probeActor1 GetProperty] SetRepresentationToPoints
  [probeActor1 GetProperty] SetPointSize 5
##########
vtkImageReader reader2
  reader2 SetDataByteOrderToLittleEndian
  reader2 SetDataExtent 0 255 0 255 1 93
  reader2 SetDataVOI 127 127 20 200 2 90
  reader2 SetFilePrefix "$VTK_DATA/fullHead/headsq"
  reader2 SetDataMask 0x7fff
  reader2 Update

vtkPlaneSource probePlane2
  probePlane2 SetOrigin 127 10 0
  probePlane2 SetPoint1 127 10 95
  probePlane2 SetPoint2 127 210 0
  probePlane2 SetResolution 20 100

vtkProbeFilter probe2
  probe2 SetInput [probePlane2 GetOutput]
  probe2 SetSource [reader2 GetOutput]
[[probe2 GetOutput] GetPointData] CopyNormalsOff
vtkPolyDataMapper probeMapper2
  probeMapper2 SetInput [probe2 GetOutput]
  eval probeMapper2 SetScalarRange 0 1200
  probeMapper2 SetLookupTable lut

vtkActor probeActor2
  probeActor2 SetMapper probeMapper2
  [probeActor2 GetProperty] SetRepresentationToPoints
  [probeActor2 GetProperty] SetPointSize 5
##########
vtkImageReader reader3
  reader3 SetDataByteOrderToLittleEndian
  reader3 SetDataExtent 0 255 0 255 1 93
  reader3 SetDataVOI 20 200 127 127 2 90
  reader3 SetFilePrefix "$VTK_DATA/fullHead/headsq"
  reader3 SetDataMask 0x7fff
  reader3 Update

vtkPlaneSource probePlane3
  probePlane3 SetOrigin 10 127 0
  probePlane3 SetPoint1 10 127 95
  probePlane3 SetPoint2 210 127 0
  probePlane3 SetResolution 100 50

vtkProbeFilter probe3
  probe3 SetInput [probePlane3 GetOutput]
  probe3 SetSource [reader3 GetOutput]

vtkPolyDataMapper probeMapper3
  probeMapper3 SetInput [probe3 GetOutput]
  eval probeMapper3 SetScalarRange 0 1200
  probeMapper3 SetLookupTable lut

vtkActor probeActor3
  probeActor3 SetMapper probeMapper3
  [probeActor3 GetProperty] SetRepresentationToPoints
  [probeActor3 GetProperty] SetPointSize 5
##########
vtkImageReader reader4
  reader4 SetDataByteOrderToLittleEndian
  reader4 SetDataExtent 0 255 0 255 1 93
  reader4 SetDataVOI 20 200 160 160 70 70
  reader4 SetFilePrefix "$VTK_DATA/fullHead/headsq"
  reader4 SetDataMask 0x7fff
  reader4 Update

vtkLineSource probeLine4
  probeLine4 SetPoint1 10 160 69
  probeLine4 SetPoint2 210 160 69
  probeLine4 SetResolution 30

vtkProbeFilter probe4
  probe4 SetInput [probeLine4 GetOutput]
  probe4 SetSource [reader4 GetOutput]
 
vtkPolyDataMapper probeMapper4
  probeMapper4 SetInput [probe4 GetOutput]
  eval probeMapper4 SetScalarRange 0 1200

vtkActor probeActor4
  probeActor4 SetMapper probeMapper4
  [probeActor4 GetProperty] SetRepresentationToPoints
  [probeActor4 GetProperty] SetPointSize 10
##########

vtkImageReader reader5
  reader5 SetDataByteOrderToLittleEndian
  reader5 SetDataExtent 0 255 0 255 1 93
  reader5 SetDataVOI 160 160 20 200 70 70
  reader5 SetFilePrefix "$VTK_DATA/fullHead/headsq"
  reader5 SetDataMask 0x7fff
  reader5 Update

vtkLineSource probeLine5
  probeLine5 SetPoint1 160 10 69
  probeLine5 SetPoint2 160 210 69
  probeLine5 SetResolution 30

vtkProbeFilter probe5
  probe5 SetInput [probeLine5 GetOutput]
  probe5 SetSource [reader5 GetOutput]
 
vtkPolyDataMapper probeMapper5
  probeMapper5 SetInput [probe5 GetOutput]
  eval probeMapper5 SetScalarRange 0 1200

vtkActor probeActor5
  probeActor5 SetMapper probeMapper5
  [probeActor5 GetProperty] SetRepresentationToPoints
  [probeActor5 GetProperty] SetPointSize 10
##########

vtkImageReader reader6
  reader6 SetDataByteOrderToLittleEndian
  reader6 SetDataExtent 0 255 0 255 1 93
  reader6 SetDataVOI 160 160 160 160 1 93
  reader6 SetFilePrefix "$VTK_DATA/fullHead/headsq"
  reader6 SetDataMask 0x7fff
  reader6 Update

vtkLineSource probeLine6
  probeLine6 SetPoint1 160 160 0
  probeLine6 SetPoint2 160 160 100
  probeLine6 SetResolution 30

vtkProbeFilter probe6
  probe6 SetInput [probeLine6 GetOutput]
  probe6 SetSource [reader6 GetOutput]
 
vtkPolyDataMapper probeMapper6
  probeMapper6 SetInput [probe6 GetOutput]
  eval probeMapper6 SetScalarRange 0 1200

vtkActor probeActor6
  probeActor6 SetMapper probeMapper6
  [probeActor6 GetProperty] SetRepresentationToPoints
  [probeActor6 GetProperty] SetPointSize 10
##########
vtkImageReader reader7
  reader7 SetDataByteOrderToLittleEndian
  reader7 SetDataExtent 0 255 0 255 1 93
  reader7 SetDataVOI 160 160 160 160 70 70
  reader7 SetFilePrefix "$VTK_DATA/fullHead/headsq"
  reader7 SetDataMask 0x7fff
  reader7 Update

vtkPoints vertexPoints
  vertexPoints SetNumberOfPoints 2
  vertexPoints InsertPoint 0 160 160 69
  vertexPoints InsertPoint 1 161 161 70

vtkCellArray aCell
  aCell InsertNextCell 1
  aCell InsertCellPoint 0
  aCell InsertNextCell 1
  aCell InsertCellPoint 1

vtkPolyData aPolyData
  aPolyData SetPoints vertexPoints
  aPolyData SetVerts aCell

vtkProbeFilter probe7
  probe7 SetInput aPolyData
  probe7 SetSource [reader7 GetOutput]
 
vtkDataSetMapper probeMapper7
  probeMapper7 SetInput [probe7 GetOutput]
  eval probeMapper7 SetScalarRange 0 1200

vtkActor probeActor7
  probeActor7 SetMapper probeMapper7
  [probeActor7 GetProperty] SetPointSize 20
  [probeActor7 GetProperty] SetRepresentationToPoints
##########

vtkOutlineFilter outline
  outline SetInput [reader1 GetOutput]
vtkPolyDataMapper outlineMapper
  outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
  outlineActor SetMapper outlineMapper
[outlineActor GetProperty] SetColor 0 0 0

ren1 AddActor outlineActor
ren1 AddActor probeActor1
ren1 AddActor probeActor2
ren1 AddActor probeActor3
ren1 AddActor probeActor4
ren1 AddActor probeActor5
ren1 AddActor probeActor6
ren1 AddActor probeActor7
ren1 SetBackground .7 .8 1
probeActor1 SetScale .8 .8 1.5
probeActor2 SetScale .8 .8 1.5
probeActor3 SetScale .8 .8 1.5
probeActor4 SetScale .8 .8 1.5
probeActor5 SetScale .8 .8 1.5
probeActor6 SetScale .8 .8 1.5
probeActor7 SetScale .8 .8 1.5
outlineActor SetScale .8 .8 1.5
#probeActor1 VisibilityOff
#probeActor2 VisibilityOff
#probeActor3 VisibilityOff
#probeActor4 VisibilityOff
#probeActor5 VisibilityOff
#probeActor6 VisibilityOff
#outlineActor VisibilityOff

set cullers [ren1 GetCullers]
$cullers InitTraversal
set culler [$cullers GetNextItem]
$culler SetMinimumCoverage 0

[ren1 GetActiveCamera] SetPosition 343.234 268.558 190.754 
[ren1 GetActiveCamera] SetFocalPoint 88 88 75 
[ren1 GetActiveCamera] SetViewAngle 30
[ren1 GetActiveCamera] SetViewUp 0 0 1
[ren1 GetActiveCamera] SetClippingRange 170.497 496.27 

renWin SetSize 500 500


iren Initialize

renWin SetFileName "probeImageData2.tcl.ppm"
#renWin SaveImageAsPPM

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .



