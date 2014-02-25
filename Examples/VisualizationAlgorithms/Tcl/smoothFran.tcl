# This example shows how to use decimation to reduce a polygonal mesh. We also
# use mesh smoothing and generate surface normals to give a pleasing result.
#

package require vtk
package require vtkinteraction

# We start by reading some data that was originally captured from
# a Cyberware laser digitizing system.
#
vtkPolyDataReader fran
    fran SetFileName "$VTK_DATA_ROOT/Data/fran_cut.vtk"

# We want to preserve topology (not let any cracks form). This may limit
# the total reduction possible, which we have specified at 90%.
#
vtkDecimatePro deci
    deci SetInputConnection [fran GetOutputPort]
    deci SetTargetReduction 0.9
    deci PreserveTopologyOn
vtkSmoothPolyDataFilter smoother
    smoother SetInputConnection [deci GetOutputPort]
    smoother SetNumberOfIterations 50
vtkPolyDataNormals normals
    normals SetInputConnection [smoother GetOutputPort]
    normals FlipNormalsOn
vtkPolyDataMapper franMapper
    franMapper SetInputConnection [normals GetOutputPort]
vtkActor franActor
    franActor SetMapper franMapper
    eval [franActor GetProperty] SetColor 1.0 0.49 0.25

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor franActor
ren1 SetBackground 1 1 1
renWin SetSize 250 250

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

vtkCamera cam1
    cam1 SetClippingRange 0.0475572 2.37786
    cam1 SetFocalPoint 0.052665 -0.129454 -0.0573973
    cam1 SetPosition 0.327637 -0.116299 -0.256418
    cam1 SetViewUp -0.0225386 0.999137 0.034901
ren1 SetActiveCamera cam1

iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .

iren Start
