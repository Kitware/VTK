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

# Create the reader for the data
# This is the data the will be volume rendered
vtkStructuredPointsReader reader
    reader SetFileName "$VTK_DATA_ROOT/Data/ironProt.vtk"

# create a reader for the other data that will
# be contoured and displayed as a polygonal mesh
vtkSLCReader reader2
    reader2 SetFileName "$VTK_DATA_ROOT/Data/neghip.slc"

# convert from vtkImageData to vtkUnstructuredGrid, remove
# any cells where all values are below 80
vtkThreshold thresh
    thresh ThresholdByUpper 80
    thresh AllScalarsOff
    thresh SetInputConnection [reader GetOutputPort]

# make sure we have only tetrahedra
vtkDataSetTriangleFilter trifilter
    trifilter SetInputConnection [thresh GetOutputPort]

# Create transfer mapping scalar value to opacity
vtkPiecewiseFunction opacityTransferFunction
    opacityTransferFunction AddPoint  80   0.0
    opacityTransferFunction AddPoint  120  0.2
    opacityTransferFunction AddPoint  255  0.2

# Create transfer mapping scalar value to color
vtkColorTransferFunction colorTransferFunction
    colorTransferFunction AddRGBPoint     80.0 0.0 0.0 0.0
    colorTransferFunction AddRGBPoint    120.0 0.0 0.0 1.0
    colorTransferFunction AddRGBPoint    160.0 1.0 0.0 0.0
    colorTransferFunction AddRGBPoint    200.0 0.0 1.0 0.0
    colorTransferFunction AddRGBPoint    255.0 0.0 1.0 1.0

# The property describes how the data will look
vtkVolumeProperty volumeProperty
    volumeProperty SetColor colorTransferFunction
    volumeProperty SetScalarOpacity opacityTransferFunction
    volumeProperty ShadeOff
    volumeProperty SetInterpolationTypeToLinear

# The mapper / ray cast function know how to render the data
vtkUnstructuredGridVolumeRayCastMapper volumeMapper
    volumeMapper SetInputConnection [trifilter GetOutputPort]

# The volume holds the mapper and the property and
# can be used to position/orient the volume
vtkVolume volume
    volume SetMapper volumeMapper
    volume SetProperty volumeProperty

# contour the second dataset
vtkContourFilter contour
    contour SetValue 0 80
    contour SetInputConnection [reader2 GetOutputPort]

# create a mapper for the polygonal data
vtkPolyDataMapper mapper
    mapper SetInputConnection [contour GetOutputPort]
    mapper ScalarVisibilityOff

# create an actor for the polygonal data
vtkActor actor
    actor SetMapper mapper

ren1 AddViewProp actor

ren1 AddVolume volume
renWin SetSize 300 300

ren1 ResetCamera
[ren1 GetActiveCamera] Azimuth 20.0
[ren1 GetActiveCamera] Elevation 10.0
[ren1 GetActiveCamera] Zoom 1.5

renWin Render


proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin AddObserver "AbortCheckEvent" {TkCheckAbort}

iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

wm withdraw .



