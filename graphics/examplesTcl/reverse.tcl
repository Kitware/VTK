catch {load vtktcl}
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/colors.tcl

# Now create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkPolyDataReader skinReader
  skinReader SetFileName "../../../vtkdata/skin.vtk"

vtkPlane plane
  plane SetNormal 1 0 0

vtkClipPolyData skinClipper
  skinClipper SetInput [skinReader GetOutput]
  skinClipper SetClipFunction plane

vtkTransform reflect
  reflect Scale -1 1 1

vtkTransformPolyDataFilter skinReflect
  skinReflect SetTransform reflect
  skinReflect SetInput [skinClipper GetOutput]

vtkReverseSense skinReverse
  skinReverse SetInput [skinReflect GetOutput]
  skinReverse ReverseNormalsOn
  skinReverse ReverseCellsOff

vtkPolyDataMapper reflectedMapper
  reflectedMapper SetInput [skinReverse GetOutput]

vtkActor reflected
  reflected SetMapper reflectedMapper
eval [reflected GetProperty] SetDiffuseColor $flesh
eval [reflected GetProperty] SetDiffuse .8
eval [reflected GetProperty] SetSpecular .5
eval [reflected GetProperty] SetSpecularPower 30
eval [reflected GetProperty] BackfaceCullingOn

ren1 AddActor reflected

vtkReverseSense skinReverse2
  skinReverse2 SetInput [skinClipper GetOutput]
  skinReverse2 ReverseNormalsOn
  skinReverse2 ReverseCellsOn

vtkPolyDataMapper skinMapper
    skinMapper SetInput [skinReverse2 GetOutput]

vtkActor skin
    skin SetMapper skinMapper

ren1 AddActor skin

ren1 SetBackground .1 .2 .4
renWin SetSize 640 512;
[ren1 GetActiveCamera] SetViewUp 0 -1 0;
[ren1 GetActiveCamera] Azimuth 180
[ren1 GetActiveCamera] Dolly 1.75
iren Initialize;

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract};

renWin SetFileName "reverse.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


