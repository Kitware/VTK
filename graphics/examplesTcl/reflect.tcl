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

vtkPolyDataNormals skinNormals
  skinNormals SetInput [skinReflect GetOutput]
  skinNormals FlipNormalsOn

vtkPolyDataMapper reflectedMapper
  reflectedMapper SetInput [skinNormals GetOutput]

vtkActor reflected
  reflected SetMapper reflectedMapper
eval [reflected GetProperty] SetDiffuseColor $flesh
eval [reflected GetProperty] SetDiffuse .8
eval [reflected GetProperty] SetSpecular .5
eval [reflected GetProperty] SetSpecularPower 30

ren1 AddActor reflected

vtkPolyDataMapper skinMapper
    skinMapper SetInput [skinClipper GetOutput]

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

#renWin SetFileName "reflect.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


