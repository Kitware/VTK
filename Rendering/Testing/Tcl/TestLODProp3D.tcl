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
    colorTransferFunction AddHSVPoint      0.0 0.01 1.0 1.0
    colorTransferFunction AddHSVPoint    127.5 0.50 1.0 1.0
    colorTransferFunction AddHSVPoint    255.0 0.99 1.0 1.0
    colorTransferFunction SetColorSpaceToHSV

# Create properties, mappers, volume actors, and ray cast function
vtkVolumeProperty volumeProperty
    volumeProperty SetColor colorTransferFunction
    volumeProperty SetScalarOpacity opacityTransferFunction
    volumeProperty SetInterpolationTypeToLinear


vtkVolumeTextureMapper2D volumeMapper
    volumeMapper SetInput [reader GetOutput]

vtkSphereSource sphereSource
    sphereSource SetCenter  25 25 25
    sphereSource SetRadius  30
    sphereSource SetThetaResolution 15
    sphereSource SetPhiResolution 15

vtkPolyDataMapper geoMapper
    geoMapper SetInput [sphereSource GetOutput]


vtkLODProp3D lod
    set geoID [lod AddLOD geoMapper 0.0]
    set volID  [lod AddLOD volumeMapper volumeProperty 0.0]

vtkProperty property
    property SetColor 1 0 0

lod SetLODProperty $geoID property

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

ren1 AddProp lod

# render a few times
renWin Render
renWin Render
renWin Render

# disable the geometry and render
lod DisableLOD $geoID
renWin Render

# disable the volume and render
lod EnableLOD $geoID
lod DisableLOD $volID
renWin Render

# chose the geometry to render
lod EnableLOD $volID
lod AutomaticLODSelectionOff
lod SetSelectedLODID $geoID
renWin Render

# choose the volume
lod SetSelectedLODID $volID
renWin Render

# this should be the volID - remove it
set id [lod GetLastRenderedLODID]
lod RemoveLOD $id

lod AutomaticLODSelectionOn
renWin Render

wm withdraw .
 
iren Initialize
