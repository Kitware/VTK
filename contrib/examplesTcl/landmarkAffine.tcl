catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

# Now create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkPolyDataReader skinReader
  skinReader SetFileName "$VTK_DATA/skin.vtk"

vtkMaskPoints maskPoints
  maskPoints SetInput [skinReader GetOutput]
  maskPoints RandomModeOn
  maskPoints SetMaximumNumberOfPoints 10

vtkMath math

vtkTransform trans
  trans RotateX [math Random -180 180]
  trans RotateY [math Random -180 180]
  trans RotateZ [math Random -180 180]
  trans Scale [math Random 1 20] [math Random 1 20] [math Random 1 20]
  trans RotateX [math Random -180 180]
  trans RotateY [math Random -180 180]
  trans RotateZ [math Random -180 180]
  trans Translate [math Random -1000 1000] [math Random -1000 1000] [math Random -1000 1000]

vtkTransformPolyDataFilter maskTrans
  maskTrans SetTransform trans
  maskTrans SetInput [maskPoints GetOutput]

vtkTransformPolyDataFilter skinTrans
  skinTrans SetTransform trans
  skinTrans SetInput [skinReader GetOutput]

maskTrans Update
maskPoints Update

vtkLandmarkTransform landTrans
  landTrans SetSourceLandmarks [[maskTrans GetOutput] GetPoints]
  landTrans SetTargetLandmarks [[maskPoints GetOutput] GetPoints]
  landTrans SetModeToAffine

vtkTransformPolyDataFilter skinBack
  skinBack SetTransform landTrans
  skinBack SetInput [skinTrans GetOutput]

vtkPolyDataNormals skinNormals
  skinNormals SetInput [skinBack GetOutput]

vtkPolyDataMapper transMapper
  transMapper SetInput [skinNormals GetOutput]

vtkActor transActor
  transActor SetMapper transMapper
  transActor AddPosition 100 0 0
  transActor AddPosition 100 0 0
eval [transActor GetProperty] SetDiffuseColor $flesh
eval [transActor GetProperty] SetDiffuse .8
eval [transActor GetProperty] SetSpecular .5
eval [transActor GetProperty] SetSpecularPower 30

ren1 AddActor transActor

vtkPolyDataMapper skinMapper
    skinMapper SetInput [skinReader GetOutput]

vtkActor skin
    skin SetMapper skinMapper

ren1 AddActor skin

ren1 SetBackground .1 .2 .4
renWin SetSize 600 300

[ren1 GetActiveCamera] SetViewUp 0 -1 0
[ren1 GetActiveCamera] Azimuth 180
[ren1 GetActiveCamera] ParallelProjectionOn
[ren1 GetActiveCamera] SetParallelScale 110

ren1 ResetCameraClippingRange

iren Initialize;

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract};

renWin SetFileName "landmark.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .
