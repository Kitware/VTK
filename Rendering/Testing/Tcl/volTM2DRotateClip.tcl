package require vtk

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
    opacityTransferFunction AddPoint  600  0.0
    opacityTransferFunction AddPoint 2000  1.0

vtkColorTransferFunction colorTransferFunction
    colorTransferFunction ClampingOff
    colorTransferFunction AddHSVPoint      0.0 0.01 1.0 1.0
    colorTransferFunction AddHSVPoint   1000.0 0.50 1.0 1.0
    colorTransferFunction AddHSVPoint   2000.0 0.99 1.0 1.0
    colorTransferFunction SetColorSpaceToHSV

# Create properties, mappers, volume actors, and ray cast function
vtkVolumeProperty volumeProperty
    volumeProperty SetColor colorTransferFunction
    volumeProperty SetScalarOpacity opacityTransferFunction

vtkVolumeTextureMapper2D volumeMapper
    volumeMapper SetInput [reader GetOutput]
    volumeMapper SetMaximumStorageSize 10000000

vtkVolume volume
    volume SetMapper volumeMapper
    volume SetProperty volumeProperty

# Create geometric sphere
vtkSphereSource sphereSource
    sphereSource SetRadius  65
    sphereSource SetThetaResolution 20
    sphereSource SetPhiResolution 40

# Compute random scalars (colors) for each cell
vtkProgrammableAttributeDataFilter randomColors
    randomColors SetInput [sphereSource GetOutput]
    randomColors SetExecuteMethod colorCells

proc colorCells {} {
    vtkMath randomColorGenerator
    set input [randomColors GetInput]
    set output [randomColors GetOutput]
    set numCells [$input GetNumberOfCells]
    vtkFloatArray colors
	colors SetNumberOfTuples $numCells

    for {set i 0} {$i < $numCells} {incr i} {
        colors SetValue $i [randomColorGenerator Random 0 1]
    }

    [$output GetCellData] CopyScalarsOff
    [$output GetCellData] PassData [$input GetCellData]
    [$output GetCellData] SetScalars colors

    colors Delete; #reference counting - it's ok
    randomColorGenerator Delete
}

vtkPolyDataMapper sphereMapper
    sphereMapper SetInput  [randomColors GetPolyDataOutput]
    
vtkActor sphereActor
    sphereActor SetMapper sphereMapper

# Set up the planes
vtkPlane plane1
plane1 SetOrigin 0 0 -10
plane1 SetNormal 0 0 1

vtkPlane plane2
plane2 SetOrigin 0 0 10
plane2 SetNormal 0 0 -1

vtkPlane plane3
plane3 SetOrigin -10 0 0 
plane3 SetNormal 1 0 0

vtkPlane plane4
plane4 SetOrigin 10 0 0
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

ren1 AddProp sphereActor
ren1 AddProp volume
ren1 SetBackground 0.1 0.2 0.4
renWin Render

[ren1 GetActiveCamera] Azimuth 45
[ren1 GetActiveCamera] Elevation 15
[ren1 GetActiveCamera] Roll 45
[ren1 GetActiveCamera] Zoom 2.0

wm withdraw .
 
iren Initialize

for { set i 0 } { $i < 5 } { incr i } {
   volume RotateY 17
   volume RotateZ 13
   sphereActor RotateX 13
   sphereActor RotateY 17
   renWin Render
}

