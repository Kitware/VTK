catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

source $VTK_TCL/vtkInt.tcl

vtkRenderer ren1
  ren1 BackingStoreOn

vtkRenderWindow renWin
renWin AddRenderer ren1

vtkRenderWindowInteractor iren
iren SetRenderWindow renWin

iren SetUserMethod {wm deiconify .vtkInteract}


vtkSLCReader reader
reader SetFileName $VTK_DATA/vw_knee.slc

reader Update

vtkPiecewiseFunction white_tfun
white_tfun AddPoint    0    1.0
white_tfun AddPoint  255    1.0


vtkPiecewiseFunction tfun
tfun AddPoint  70     0.0
tfun AddPoint  80     1.0 

ren1 SetBackground .1 .2 .4

vtkVolumeProperty vol_prop
vol_prop SetColor white_tfun
vol_prop SetScalarOpacity tfun
vol_prop SetInterpolationTypeToLinear
vol_prop ShadeOn

vtkVolumeRayCastCompositeFunction comp_func

vtkVolumeRayCastMapper  volmap
volmap SetVolumeRayCastFunction comp_func
volmap SetInput [reader GetOutput]
volmap SetSampleDistance 1.0

vtkVolume vol
vol SetProperty vol_prop
vol SetMapper volmap

ren1 AddVolume vol

vtkImageShrink3D shrink
  shrink SetInput [reader GetOutput]
  shrink SetShrinkFactors 4 4 2
  shrink AveragingOn

vtkContourFilter contour
  contour SetInput [shrink GetOutput]
  contour SetValue 0 30.0

vtkPoints points
  points InsertPoint 0 100.0 150.0 130.0
  points InsertPoint 1 100.0 150.0 130.0
  points InsertPoint 2 100.0 150.0 130.0

vtkNormals normals
  normals InsertNormal 0 1.0 0.0 0.0
  normals InsertNormal 1 0.0 1.0 0.0
  normals InsertNormal 2 0.0 0.0 1.0
  

vtkPlanes planes
  planes SetPoints points
  planes SetNormals normals
  

vtkClipPolyData clipper
  clipper SetInput [contour GetOutput]
  clipper SetClipFunction planes
  clipper GenerateClipScalarsOn

vtkPolyDataMapper skin_mapper
  skin_mapper SetInput [clipper GetOutput]
  skin_mapper ScalarVisibilityOff

vtkActor skin
  skin SetMapper skin_mapper
  [skin GetProperty] SetColor 0.8 0.4 0.2

ren1 AddActor skin

renWin SetSize 200 200


[ren1 GetActiveCamera] SetPosition -47.5305 -319.315 92.0083
[ren1 GetActiveCamera] SetFocalPoint 78.9121 89.8372 95.1229
[ren1 GetActiveCamera] SetViewUp -0.00708891 0.00980254 -0.999927
[ren1 GetActiveCamera] SetClippingRange 42.8255 2141.28

iren Initialize

renWin Render

wm withdraw .

