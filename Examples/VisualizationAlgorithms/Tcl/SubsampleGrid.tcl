# This example demonstrates the subsampling of a structured grid.

#
# First we include the VTK Tcl packages which will make available
# all of the vtk commands from Tcl. The vtkinteraction package defines
# a simple Tcl/Tk interactor widget.
#
package require vtk
package require vtkinteraction

# Read some structured data.
#
vtkMultiBlockPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update

set pl3dOutput [[pl3d GetOutput] GetBlock 0]

# Here we subsample the grid. The SetVOI method requires six values
# specifying (imin,imax, jmin,jmax, kmin,kmax) extents. In this example
# we extracting a plane. Note that the VOI is clamped to zero (min) and
# the maximum i-j-k value; that way we can use the -1000,1000 specification
# and be sure the values are clamped. The SampleRate specifies that we take
# every point in the i-direction; every other point in the j-direction; and
# every third point in the k-direction. IncludeBoundaryOn makes sure that we
# get the boundary points even if the SampleRate does not coincident with
# the boundary.
#
vtkExtractGrid extract
    extract SetInputData $pl3dOutput
    extract SetVOI 30 30 -1000 1000 -1000 1000
    extract SetSampleRate 1 2 3
    extract IncludeBoundaryOn
vtkDataSetMapper mapper
    mapper SetInputConnection [extract GetOutputPort]
    mapper SetScalarRange .18 .7
vtkActor actor
    actor SetMapper mapper

vtkStructuredGridOutlineFilter outline
    outline SetInputData $pl3dOutput
vtkPolyDataMapper outlineMapper
    outlineMapper SetInputConnection [outline GetOutputPort]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    [outlineActor GetProperty] SetColor 0 0 0

# Add the usual rendering stuff.
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor actor

ren1 SetBackground 1 1 1
renWin SetSize 300 180

set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 2.64586 47.905
$cam1 SetFocalPoint 8.931 0.358127 31.3526
$cam1 SetPosition 29.7111 -0.688615 37.1495
$cam1 SetViewUp -0.268328 0.00801595 0.963294

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .
