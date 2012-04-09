package require vtk
package require vtkinteraction

# create a rendering window and renderer
vtkRenderer ren1

vtkRenderWindow renWin
renWin AddRenderer ren1
renWin StereoCapableWindowOn

vtkRenderWindowInteractor iren
iren SetRenderWindow renWin

vtkGenericEnSightReader reader
# Make sure all algorithms use the composite data pipeline
vtkCompositeDataPipeline cdp
reader SetDefaultExecutivePrototype cdp
reader SetCaseFileName "$VTK_DATA_ROOT/Data/EnSight/TEST_bin.case"

vtkDataSetSurfaceFilter dss
dss SetInputConnection [reader GetOutputPort]

vtkHierarchicalPolyDataMapper mapper
mapper SetInputConnection [dss GetOutputPort]
mapper SetColorModeToMapScalars
mapper SetScalarModeToUseCellFieldData
mapper ColorByArrayComponent "Pressure" 0
mapper SetScalarRange 0.121168 0.254608

vtkActor actor
actor SetMapper mapper

# assign our actor to the renderer
ren1 AddActor actor

# enable user interface interactor
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

[ren1 GetActiveCamera] SetPosition 0.643568 0.424804 -0.477458
[ren1 GetActiveCamera] SetFocalPoint 0.894177 0.490735 0.028153
[ren1 GetActiveCamera] SetViewAngle 30
[ren1 GetActiveCamera] SetViewUp 0.338885 0.896657 -0.284892
ren1 ResetCameraClippingRange

renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .

reader SetDefaultExecutivePrototype {}
