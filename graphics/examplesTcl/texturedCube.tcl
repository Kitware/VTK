catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


source $VTK_TCL/vtkInt.tcl

vtkRenderer ren1
vtkRenderWindow renWin
	renWin AddRenderer ren1
vtkRenderWindowInteractor iren
	iren SetRenderWindow renWin

vtkTIFFReader tiff
	tiff SetFileName "$VTK_DATA/vtk.tif"

vtkTexture atext
	atext SetInput [tiff GetOutput]
	atext InterpolateOn

vtkCubeSource cube 
	cube SetXLength 3
	cube SetYLength 2
	cube SetZLength 1

vtkPolyDataMapper cubeMapper
	cubeMapper SetInput [cube GetOutput]
vtkActor cubeActor
	cubeActor SetMapper cubeMapper
	cubeActor SetTexture atext

ren1 AddActor cubeActor
ren1 SetBackground 0.2 0.3 0.4

renWin SetSize 400 400

[ren1 GetActiveCamera] Azimuth 45
[ren1 GetActiveCamera] Elevation 30 
ren1 ResetCameraClippingRange

renWin Render

iren SetUserMethod {wm deiconify .vtkInteract}

wm withdraw .

