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

vtkPolyDataMapper skinMapper
  skinMapper SetInput [skinReader GetOutput]

vtkActor skin
  skin SetMapper skinMapper
eval [skin GetProperty] SetDiffuseColor $flesh
eval [skin GetProperty] SetDiffuse .8
eval [skin GetProperty] SetSpecular .5
eval [skin GetProperty] SetSpecularPower 30

ren1 AddActor skin

vtkPlane aPlane
    aPlane SetOrigin 0 1.5 0
    aPlane SetNormal 0 1 0

vtkCutter aCutPlane
    aCutPlane SetInput [skinReader GetOutput]
    aCutPlane SetCutFunction aPlane
    aCutPlane GenerateValues 23 1.5 139.5

vtkStripper aStripper
    aStripper SetInput [aCutPlane GetOutput]

vtkTubeFilter tubes
    tubes SetInput [aStripper GetOutput]
    tubes SetNumberOfSides 8
    tubes UseDefaultNormalOn
    tubes SetDefaultNormal 0 1 0

vtkPolyDataMapper cutPlaneMapper
    cutPlaneMapper SetInput [tubes GetOutput]
    cutPlaneMapper SetScalarRange -100 100

vtkActor cut
    cut SetMapper cutPlaneMapper

ren1 AddActor cut

ren1 SetBackground 1 1 1
renWin SetSize 640 512;
[ren1 GetActiveCamera] SetViewUp 0 -1 0;
[ren1 GetActiveCamera] Azimuth 230
[ren1 GetActiveCamera] Elevation 30
[ren1 GetActiveCamera] Dolly 1.75
ren1 ResetCameraClippingRange

iren Initialize;

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract};

#renWin SetFileName "cutModel.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


