# This example shows how to extract a piece of a dataset using an implicit 
# function. In this case the implicit function is formed by the boolean 
# combination of two ellipsoids.
#

package require vtk
package require vtkinteraction

# Here we create two ellipsoidal implicit functions and boolean them
# together tto form a "cross" shaped implicit function.
vtkQuadric quadric
    quadric SetCoefficients .5 1 .2 0 .1 0 0 .2 0 0
vtkSampleFunction sample
    sample SetSampleDimensions 50 50 50
    sample SetImplicitFunction quadric
    sample ComputeNormalsOff
vtkTransform trans
    trans Scale 1 .5 .333
vtkSphere sphere
    sphere SetRadius 0.25
    sphere SetTransform trans
vtkTransform trans2
    trans2 Scale .25 .5 1.0
vtkSphere sphere2
    sphere2 SetRadius 0.25
    sphere2 SetTransform trans2
vtkImplicitBoolean union
    union AddFunction sphere
    union AddFunction sphere2
    union SetOperationType 0;#union


# Here is where it gets interesting. The implicit function is used to
# extract those cells completely inside the function. They are then 
# shrunk to help show what was extracted.
vtkExtractGeometry extract
    extract SetInputConnection [sample GetOutputPort]
    extract SetImplicitFunction union
vtkShrinkFilter shrink
    shrink SetInputConnection [extract GetOutputPort]
    shrink SetShrinkFactor 0.5
vtkDataSetMapper dataMapper
    dataMapper SetInputConnection [shrink GetOutputPort]
vtkActor dataActor
    dataActor SetMapper dataMapper

# The outline gives context to the original data.
vtkOutlineFilter outline
    outline SetInputConnection [sample GetOutputPort]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInputConnection [outline GetOutputPort]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
set outlineProp [outlineActor GetProperty]
eval $outlineProp SetColor 0 0 0

# The usual rendering stuff is created.
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor dataActor
ren1 SetBackground 1 1 1
renWin SetSize 500 500
ren1 ResetCamera
[ren1 GetActiveCamera] Zoom 1.5
iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .
