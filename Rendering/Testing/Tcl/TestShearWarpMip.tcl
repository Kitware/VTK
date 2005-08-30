# This is a simple volume rendering example that
# uses a vtkVolumeRayCast mapper

package require vtk
package require vtkinteraction

# Simple volume rendering example.
vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 63 0 63 1 93
reader SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
reader SetDataMask 0x7fff
reader SetDataSpacing 2 2 1
reader SetDataScalarTypeToUnsignedShort
reader Update

[reader GetOutput] SetOrigin -63 -63 -46

# Create transfer functions for opacity and color
vtkPiecewiseFunction opacityTransferFunction
    opacityTransferFunction AddPoint  100  0.0
    opacityTransferFunction AddPoint 2000  1.0

vtkColorTransferFunction colorTransferFunction
    colorTransferFunction ClampingOff
    colorTransferFunction AddRGBPoint      0.0 1.00 0.0 0.0
    colorTransferFunction AddRGBPoint   1000.0 0.00 1.0 0.0
    colorTransferFunction AddRGBPoint   2000.0 0.00 0.0 1.0
    colorTransferFunction AddRGBPoint   3000.0 1.00 1.0 1.0
    colorTransferFunction AddRGBPoint   4000.0 1.00 1.0 1.0
#    colorTransferFunction SetColorSpaceToHSV

# Create properties, mappers, volume actors, and ray cast function
vtkVolumeProperty volumeProperty
    volumeProperty SetColor colorTransferFunction
    volumeProperty SetScalarOpacity opacityTransferFunction
    volumeProperty ShadeOn
    volumeProperty SetInterpolationTypeToLinear 

# The mapper / ray cast function know how to render the data
vtkOpenGLVolumeShearWarpMapper  volumeMapper
   volumeMapper SetInputConnection [reader GetOutputPort]
	volumeMapper FastClassificationOff
	volumeMapper SetFunctionTypeToMIP
volumeMapper IntermixIntersectingGeometryOn

# The volume holds the mapper and the property and
# can be used to position/orient the volume
vtkVolume volume
    volume SetMapper volumeMapper
    volume SetProperty volumeProperty

	vtkCamera cam
	cam ParallelProjectionOn

# Create the standard renderer, render window
# and interactor
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 SetActiveCamera cam
#ren1 AddVolume volume
ren1 SetBackground 0.5 0.5 0.5
renWin SetSize 600 600

ren1 AddVolume volume
ren1 ResetCamera
renWin Render

proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin AddObserver AbortCheckEvent {TkCheckAbort}

iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

wm withdraw .



