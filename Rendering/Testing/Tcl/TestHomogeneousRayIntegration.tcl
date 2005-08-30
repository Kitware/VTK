package require vtk
package require vtkinteraction

# Create the standard renderer, render window
# and interactor
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin
    iren SetDesiredUpdateRate 3

# Create a small mesh.  The coarser and more opaque the mesh, the easier it
# is to see rendering errors.
vtkImageMandelbrotSource input
    input SetWholeExtent 0 2 0 2 0 2
    input SetSizeCX 2 2 2 2
    input SetMaximumNumberOfIterations 10

# make sure we have only tetrahedra
vtkDataSetTriangleFilter trifilter
    trifilter SetInputConnection [input GetOutputPort]

# Convert it to cell centered data.
vtkPointDataToCellData celldata
    celldata SetInputConnection [trifilter GetOutputPort]
    celldata PassPointDataOff

# Create transfer mapping scalar value to opacity
vtkPiecewiseFunction opacityTransferFunction
    opacityTransferFunction AddPoint  0   0.0
    opacityTransferFunction AddPoint  10  1.0

# Create transfer mapping scalar value to color
vtkColorTransferFunction colorTransferFunction
    colorTransferFunction AddRGBPoint     0  1.0 0.0 1.0
    colorTransferFunction AddRGBPoint     2  0.0 0.0 1.0
    colorTransferFunction AddRGBPoint     4  0.0 1.0 1.0
    colorTransferFunction AddRGBPoint     6  0.0 1.0 0.0
    colorTransferFunction AddRGBPoint     8  1.0 1.0 0.0
    colorTransferFunction AddRGBPoint    10  1.0 0.0 0.0

# The property describes how the data will look
vtkVolumeProperty volumeProperty
    volumeProperty SetColor colorTransferFunction
    volumeProperty SetScalarOpacity opacityTransferFunction
    volumeProperty ShadeOff
    volumeProperty SetInterpolationTypeToLinear
    volumeProperty SetScalarOpacityUnitDistance 0.75

# The mapper / ray cast function / ray integrator know how to render the data
vtkUnstructuredGridVolumeRayCastMapper volumeMapper
    volumeMapper SetInputConnection [celldata GetOutputPort]

vtkUnstructuredGridLinearRayIntegrator rayIntegrator
    volumeMapper SetRayIntegrator rayIntegrator

# The volume holds the mapper and the property and
# can be used to position/orient the volume
vtkVolume volume
    volume SetMapper volumeMapper
    volume SetProperty volumeProperty

ren1 AddVolume volume
renWin SetSize 300 300

[ren1 GetActiveCamera] Azimuth 20.0
[ren1 GetActiveCamera] Elevation 15.0
[ren1 GetActiveCamera] Zoom 1.5

renWin Render


proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin AddObserver AbortCheckEvent {TkCheckAbort}

iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

wm withdraw .



