# This example demonstrates how to use boolean combinations of implicit
# functions to create a model of an ice cream cone.

# first we load in the standard vtk packages into tcl
package require vtk
package require vtkinteraction
package require vtktesting

# Create implicit function primitives. These have been carefully placed to
# give the effect that we want. We are going to use various combinations of
# these functions to create the shape we want; for example, we use planes
# intersected with a cone (which is infinite in extent) to get a finite
# cone.
#
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

# Combine primitives to build ice-cream cone. Clip the cone with planes.
vtkImplicitBoolean theCone
    theCone SetOperationTypeToIntersection
    theCone AddFunction cone
    theCone AddFunction vertPlane
    theCone AddFunction basePlane

# Take a bite out of the ice cream.
vtkImplicitBoolean theCream
    theCream SetOperationTypeToDifference
    theCream AddFunction iceCream
    theCream AddFunction bite

# The sample function generates a distance function from the 
# implicit function (which in this case is the cone). This is 
# then contoured to get a polygonal surface.
#
vtkSampleFunction theConeSample
    theConeSample SetImplicitFunction theCone
    theConeSample SetModelBounds -1 1.5 -1.25 1.25 -1.25 1.25 
    theConeSample SetSampleDimensions 60 60 60
    theConeSample ComputeNormalsOff
vtkContourFilter theConeSurface
    theConeSurface SetInput [theConeSample GetOutput]
    theConeSurface SetValue 0 0.0
vtkPolyDataMapper coneMapper
    coneMapper SetInput [theConeSurface GetOutput]
    coneMapper ScalarVisibilityOff
vtkActor coneActor
    coneActor SetMapper coneMapper
    eval [coneActor GetProperty] SetColor $chocolate

# The same here for the ice cream.
#
vtkSampleFunction theCreamSample
    theCreamSample SetImplicitFunction theCream
    theCreamSample SetModelBounds  0 2.5 -1.25 1.25 -1.25 1.25 
    theCreamSample SetSampleDimensions 60 60 60
    theCreamSample ComputeNormalsOff
vtkContourFilter theCreamSurface
    theCreamSurface SetInput [theCreamSample GetOutput]
    theCreamSurface SetValue 0 0.0
vtkPolyDataMapper creamMapper
    creamMapper SetInput [theCreamSurface GetOutput]
    creamMapper ScalarVisibilityOff
vtkActor creamActor
    creamActor SetMapper creamMapper
    eval [creamActor GetProperty] SetColor $mint

# Create the usual rendering stuff
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor coneActor
ren1 AddActor creamActor
ren1 SetBackground 1 1 1
renWin SetSize 250 250
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
