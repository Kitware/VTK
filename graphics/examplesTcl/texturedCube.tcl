catch {load vtktcl}

source ../../examplesTcl/vtkInt.tcl

vtkRenderer ren1
vtkRenderWindow renWin
	renWin AddRenderer ren1
vtkRenderWindowInteractor iren
	iren SetRenderWindow renWin

vtkTIFFReader tiff
	tiff SetFileName "../../../vtkdata/vtk.tif"

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

renWin Render

iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

wm withdraw .

