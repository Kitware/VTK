# This example demonstrates how to extract "computational planes" from a
# structured dataset. Structured data has a natural, logical coordinate
# system based on i-j-k indices. Specifying imin,imax, jmin,jmax, kmin,kmax
# pairs can indicate a point, line, plane, or volume of data.
#
# In this example, we extract three planes and warp them using scalar values
# in the direction of the local normal at each point. This gives a sort of
# "velocity profile" that indicates the nature of the flow.

#
# First we include the VTK Tcl packages which will make available
# all of the vtk commands from Tcl. The vtkinteraction package defines
# a simple Tcl/Tk interactor widget.
#
package require vtk
package require vtkinteraction

# Here we read data from a annular combustor. A combustor burns fuel and air
# in a gas turbine (e.g., a jet engine) and the hot gas eventually makes its
# way to the turbine section.
#
vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update

# Planes are specified using a imin,imax, jmin,jmax, kmin,kmax coordinate
# specification. Min and max i,j,k values are clamped to 0 and maximum value.
#
vtkStructuredGridGeometryFilter plane
    plane SetInputConnection [pl3d GetOutputPort]
    plane SetExtent 10 10 1 100 1 100
vtkStructuredGridGeometryFilter plane2
    plane2 SetInputConnection [pl3d GetOutputPort]
    plane2 SetExtent 30 30 1 100 1 100
vtkStructuredGridGeometryFilter plane3
    plane3 SetInputConnection [pl3d GetOutputPort]
    plane3 SetExtent 45 45 1 100 1 100

# We use an append filter because that way we can do the warping, etc. just
# using a single pipeline and actor.
#
vtkAppendPolyData appendF
    appendF AddInputConnection [plane GetOutputPort]
    appendF AddInputConnection [plane2 GetOutputPort]
    appendF AddInputConnection [plane3 GetOutputPort]
vtkWarpScalar warp
    warp SetInputConnection [appendF GetOutputPort]
    warp UseNormalOn
    warp SetNormal 1.0 0.0 0.0
    warp SetScaleFactor 2.5
vtkPolyDataNormals normals
    normals SetInputConnection [warp GetOutputPort]
    normals SetFeatureAngle 60
vtkPolyDataMapper planeMapper
    planeMapper SetInputConnection [normals GetOutputPort]
    eval planeMapper SetScalarRange [[pl3d GetOutput] GetScalarRange]
vtkActor planeActor
    planeActor SetMapper planeMapper

# The outline provides context for the data and the planes.
vtkStructuredGridOutlineFilter outline
    outline SetInputConnection [pl3d GetOutputPort]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInputConnection [outline GetOutputPort]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    [outlineActor GetProperty] SetColor 0 0 0

# Create the usual graphics stuff/
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor outlineActor
ren1 AddActor planeActor
ren1 SetBackground 1 1 1
renWin SetSize 500 500

# Create an initial view.
set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 3.95297 50
$cam1 SetFocalPoint 8.88908 0.595038 29.3342
$cam1 SetPosition -12.3332 31.7479 41.2387
$cam1 SetViewUp 0.060772 -0.319905 0.945498
iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .



