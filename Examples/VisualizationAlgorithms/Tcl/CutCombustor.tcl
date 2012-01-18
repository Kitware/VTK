# This example shows how to use cutting (vtkCutter) and how it compares
# with extracting a plane from a computational grid.
#

package require vtk
package require vtkinteraction

# Read some data.
vtkMultiBlockPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update

set pl3dOutput [[pl3d GetOutput] GetBlock 0 ]

# The cutter uses an implicit function to perform the cutting.
# Here we define a plane, specifying its center and normal.
# Then we assign the plane to the cutter.
vtkPlane plane
    eval plane SetOrigin [$pl3dOutput GetCenter]
    plane SetNormal -0.287 0 0.9579
vtkCutter planeCut
    planeCut SetInputData $pl3dOutput
    planeCut SetCutFunction plane
vtkPolyDataMapper cutMapper
    cutMapper SetInputConnection [planeCut GetOutputPort]
    eval cutMapper SetScalarRange \
      [[[$pl3dOutput GetPointData] GetScalars] GetRange]
vtkActor cutActor
    cutActor SetMapper cutMapper

# Here we extract a computational plane from the structured grid.
# We render it as wireframe.
vtkStructuredGridGeometryFilter compPlane
    compPlane SetInputData $pl3dOutput
    compPlane SetExtent 0 100 0 100 9 9
vtkPolyDataMapper planeMapper
    planeMapper SetInputConnection [compPlane GetOutputPort]
    planeMapper ScalarVisibilityOff
vtkActor planeActor
    planeActor SetMapper planeMapper
    [planeActor GetProperty] SetRepresentationToWireframe
    [planeActor GetProperty] SetColor 0 0 0

# The outline of the data puts the data in context.
vtkStructuredGridOutlineFilter outline
    outline SetInputData $pl3dOutput
vtkPolyDataMapper outlineMapper
    outlineMapper SetInputConnection [outline GetOutputPort]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
set outlineProp [outlineActor GetProperty]
eval $outlineProp SetColor 0 0 0

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor planeActor
ren1 AddActor cutActor

ren1 SetBackground 1 1 1
renWin SetSize 400 300

set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 11.1034 59.5328
$cam1 SetFocalPoint 9.71821 0.458166 29.3999
$cam1 SetPosition -2.95748 -26.7271 44.5309
$cam1 SetViewUp 0.0184785 0.479657 0.877262
iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .



