package require vtk
package require vtkinteraction

# Simple volume rendering example.
vtkSLCReader reader
    reader SetFileName "$VTK_DATA_ROOT/Data/sphere.slc"

# Create transfer functions for opacity and color
vtkPiecewiseFunction opacityTransferFunction
    opacityTransferFunction AddPoint    0  0.0
    opacityTransferFunction AddPoint   30  0.0
    opacityTransferFunction AddPoint   80  0.5
    opacityTransferFunction AddPoint  255  0.5

vtkColorTransferFunction colorTransferFunction
    colorTransferFunction AddRGBPoint      0.0 0.0 0.0 0.0
    colorTransferFunction AddRGBPoint     64.0 1.0 0.0 0.0
    colorTransferFunction AddRGBPoint    128.0 0.0 0.0 1.0
    colorTransferFunction AddRGBPoint    192.0 0.0 1.0 0.0
    colorTransferFunction AddRGBPoint    255.0 0.0 0.2 0.0

# Create properties, mappers, volume actors, and ray cast function
vtkVolumeProperty volumeProperty
    volumeProperty SetColor colorTransferFunction
    volumeProperty SetScalarOpacity opacityTransferFunction
    volumeProperty SetInterpolationTypeToLinear
    volumeProperty ShadeOn

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin SetSize 300 300
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

renWin SetMultiSamples 0

ren1 SetBackground 0.1 0.2 0.4

# Translate the volume to center it at (0,0,0)
vtkTransform userTrans
    userTrans PostMultiply
    userTrans Identity
    userTrans Translate -24.5 -24.5 -24.5

# Clipping planes are in world coords
vtkPlane plane1
    plane1 SetOrigin -24 0 0
    plane1 SetNormal 1 0 0

vtkPlane plane2
    plane2 SetOrigin 24 0 0
    plane2 SetNormal -1 0 0

vtkPlane plane3
    plane3 SetOrigin 0 -15 0
    plane3 SetNormal 0.163176 0.925417 -0.342020

vtkPlane plane4
    plane4 SetOrigin 0 24 0
    plane4 SetNormal 0 -1 0

vtkPlane plane5
    plane5 SetOrigin 0 0 -24
    plane5 SetNormal 0 0 1

vtkPlane plane6
    plane6 SetOrigin 0 0 24
    plane6 SetNormal 0 0 -1

vtkPlaneCollection clippingPlanes
    clippingPlanes AddItem plane1
    clippingPlanes AddItem plane2
    clippingPlanes AddItem plane3
    clippingPlanes AddItem plane4
    clippingPlanes AddItem plane5
    clippingPlanes AddItem plane6

# Cropping planes are in data coords
vtkVolumeTextureMapper2D volumeMapper1
    volumeMapper1 SetInputConnection [reader GetOutputPort]
    volumeMapper1 CroppingOn
    volumeMapper1 SetCroppingRegionPlanes 16 33 16 33 16 33
    volumeMapper1 SetClippingPlanes clippingPlanes

vtkVolume volume1
    volume1 SetMapper volumeMapper1
    volume1 SetProperty volumeProperty

vtkVolumeOutlineSource outline1
    outline1 SetVolumeMapper volumeMapper1
    outline1 GenerateFacesOn
    outline1 GenerateScalarsOn

vtkTransformPolyDataFilter preTrans1
    preTrans1 SetInputConnection [outline1 GetOutputPort]
    preTrans1 SetTransform userTrans

vtkClipClosedSurface outlineClip1
    outlineClip1 SetInputConnection [preTrans1 GetOutputPort]
    outlineClip1 SetClippingPlanes clippingPlanes
    outlineClip1 GenerateFacesOff
    outlineClip1 GenerateOutlineOn
    outlineClip1 SetScalarModeToColors
    outlineClip1 SetClipColor 1 1 0
    outlineClip1 SetActivePlaneId 2
    outlineClip1 SetActivePlaneColor 0 1 0

vtkTransformPolyDataFilter postTrans1
    postTrans1 SetInputConnection [outlineClip1 GetOutputPort]
    postTrans1 SetTransform [userTrans GetInverse]

vtkDataSetMapper outlineMapper1
    outlineMapper1 SetInputConnection [postTrans1 GetOutputPort]

vtkActor outlineActor1
    outlineActor1 SetMapper outlineMapper1

volume1 SetUserTransform userTrans
outlineActor1 SetUserTransform userTrans

ren1 AddViewProp outlineActor1
ren1 AddViewProp volume1

volumeMapper1 SetCroppingRegionFlagsToFence

outline1 GenerateScalarsOn

ren1 ResetCamera
[ren1 GetActiveCamera] Zoom 1.35
renWin Render

iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

wm withdraw .



