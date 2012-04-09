package require vtk
package require vtkinteraction
package require vtktesting

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create implicit function primitives
vtkCone cone
    cone SetAngle 20
vtkPlane vertPlane
    vertPlane SetOrigin .1 0 0
    vertPlane SetNormal -1 0 0
vtkPlane basePlane
    basePlane SetOrigin 1.2 0 0
    basePlane SetNormal 1 0 0
vtkSphere iceCream
    iceCream SetCenter 1.333 0 0
    iceCream SetRadius 0.5
vtkSphere bite
    bite SetCenter 1.5 0 0.5
    bite SetRadius 0.25

# combine primitives to build ice-cream cone
vtkImplicitBoolean theCone
    theCone SetOperationTypeToIntersection
    theCone AddFunction cone
    theCone AddFunction vertPlane
    theCone AddFunction basePlane

vtkImplicitBoolean theCream
    theCream SetOperationTypeToDifference
    theCream AddFunction iceCream
    theCream AddFunction bite

# iso-surface to create geometry
vtkSampleFunction theConeSample
    theConeSample SetImplicitFunction theCone
    theConeSample SetModelBounds -1 1.5 -1.25 1.25 -1.25 1.25 
    theConeSample SetSampleDimensions 60 60 60
    theConeSample ComputeNormalsOff
vtkMarchingContourFilter theConeSurface
    theConeSurface SetInputConnection [theConeSample GetOutputPort]
    theConeSurface SetValue 0 0.0
vtkPolyDataMapper coneMapper
    coneMapper SetInputConnection [theConeSurface GetOutputPort]
    coneMapper ScalarVisibilityOff
vtkActor coneActor
    coneActor SetMapper coneMapper
    eval [coneActor GetProperty] SetColor $chocolate

# iso-surface to create geometry
vtkSampleFunction theCreamSample
    theCreamSample SetImplicitFunction theCream
    theCreamSample SetModelBounds  0 2.5 -1.25 1.25 -1.25 1.25 
    theCreamSample SetSampleDimensions 60 60 60
    theCreamSample ComputeNormalsOff
vtkMarchingContourFilter theCreamSurface
    theCreamSurface SetInputConnection [theCreamSample GetOutputPort]
    theCreamSurface SetValue 0 0.0
vtkPolyDataMapper creamMapper
    creamMapper SetInputConnection [theCreamSurface GetOutputPort]
    creamMapper ScalarVisibilityOff
vtkActor creamActor
    creamActor SetMapper creamMapper
    eval [creamActor GetProperty] SetColor $mint

# Add the actors to the renderer, set the background and size
#
ren1 AddActor coneActor
ren1 AddActor creamActor
ren1 SetBackground 1 1 1
renWin SetSize 250 250
ren1 ResetCamera
[ren1 GetActiveCamera] Roll 90
[ren1 GetActiveCamera] Dolly 1.5
ren1 ResetCameraClippingRange
iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .
