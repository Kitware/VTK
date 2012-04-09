package require vtk

# Simple volume rendering example.
vtkSLCReader reader
    reader SetFileName "$VTK_DATA_ROOT/Data/sphere.slc"

reader Update

# Create transfer functions for opacity and color
vtkPiecewiseFunction opacityTransferFunction
    opacityTransferFunction AddPoint  20   0.0
    opacityTransferFunction AddPoint  255  1.0

vtkColorTransferFunction colorTransferFunction

# Improve coverage
    colorTransferFunction SetColorSpaceToRGB
    colorTransferFunction AddRGBPoint 100 1 1 1
    colorTransferFunction AddRGBPoint   0 0 0 0
    colorTransferFunction AddRGBPoint 200 1 0 1
    colorTransferFunction AddRGBPoint 100 0 0 0
    colorTransferFunction RemovePoint 100
    colorTransferFunction RemovePoint   0
    colorTransferFunction RemovePoint 200
    colorTransferFunction AddHSVPoint 100 1 1 1
    colorTransferFunction AddHSVPoint   0 0 0 0
    colorTransferFunction AddHSVPoint 200 1 0 1
    colorTransferFunction AddHSVPoint 100 0 0 0
    colorTransferFunction RemovePoint   0
    colorTransferFunction RemovePoint 200
    colorTransferFunction RemovePoint 100
    colorTransferFunction AddRGBSegment   0 1 1 1 100 0 0 0
    colorTransferFunction AddRGBSegment  50 1 1 1 150 0 0 0
    colorTransferFunction AddRGBSegment  60 1 1 1  90 0 0 0
    colorTransferFunction AddHSVSegment  90 1 1 1 105 0 0 0
    colorTransferFunction AddHSVSegment  40 1 1 1 155 0 0 0
    colorTransferFunction AddHSVSegment  30 1 1 1  95 0 0 0


    colorTransferFunction RemoveAllPoints
    colorTransferFunction AddHSVPoint      0.0 0.01 1.0 1.0
    colorTransferFunction AddHSVPoint    127.5 0.50 1.0 1.0
    colorTransferFunction AddHSVPoint    255.0 0.99 1.0 1.0
    colorTransferFunction SetColorSpaceToHSV

# Create properties, mappers, volume actors, and ray cast function
vtkVolumeProperty volumeProperty
    volumeProperty SetColor colorTransferFunction
    volumeProperty SetScalarOpacity opacityTransferFunction
    volumeProperty SetInterpolationTypeToLinear

vtkVolumeRayCastCompositeFunction  compositeFunction

vtkVolumeRayCastMapper volumeMapper
    volumeMapper SetInputConnection [reader GetOutputPort]
    volumeMapper SetVolumeRayCastFunction compositeFunction

vtkVolume volume
    volume SetMapper volumeMapper
    volume SetProperty volumeProperty

# Create geometric sphere
vtkSphereSource sphereSource
    sphereSource SetCenter  25 25 25
    sphereSource SetRadius  30
    sphereSource SetThetaResolution 15
    sphereSource SetPhiResolution 15

vtkPolyDataMapper sphereMapper
    sphereMapper SetInputConnection [sphereSource GetOutputPort]

vtkActor sphereActor
    sphereActor SetMapper sphereMapper

# Set up the planes
vtkPlane plane1
plane1 SetOrigin 25 25 20
plane1 SetNormal 0 0 1

vtkPlane plane2
plane2 SetOrigin 25 25 30
plane2 SetNormal 0 0 -1

vtkPlane plane3
plane3 SetOrigin 20 25 25
plane3 SetNormal 1 0 0

vtkPlane plane4
plane4 SetOrigin 30 25 25
plane4 SetNormal -1 0 0

sphereMapper AddClippingPlane plane1
sphereMapper AddClippingPlane plane2

volumeMapper AddClippingPlane plane3
volumeMapper AddClippingPlane plane4


# Okay now the graphics stuff
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin SetSize 256 256
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

[ren1 GetCullers] InitTraversal
set culler [[ren1 GetCullers] GetNextItem]
$culler SetSortingStyleToBackToFront

ren1 AddViewProp sphereActor
ren1 AddViewProp volume
ren1 SetBackground 0.1 0.2 0.4
renWin Render

[ren1 GetActiveCamera] Azimuth 45
[ren1 GetActiveCamera] Elevation 15
[ren1 GetActiveCamera] Roll 45
[ren1 GetActiveCamera] Zoom 2.0

wm withdraw .

iren Initialize
