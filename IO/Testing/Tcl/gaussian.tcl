package require vtk
package require vtkinteraction

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin SetSize 300 300
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Create the reader for the data
#vtkStructuredPointsReader reader
vtkGaussianCubeReader reader
    reader SetFileName "$VTK_DATA_ROOT/Data/m4_TotalDensity.cube"
    reader SetHBScale 1.1
    reader SetBScale 1.9
    reader Update

set range [[[[reader GetOutput 1] GetPointData] GetScalars] GetRange]
set min [lindex $range 0]
set max [lindex $range 1]

set xform [reader GetTransform]

vtkImageShiftScale readerSS
  readerSS SetInput [reader GetOutput 1]
  readerSS SetShift [expr $min * -1]
  readerSS SetScale [expr 255 / [expr $max - $min]]
  readerSS SetOutputScalarTypeToUnsignedChar

vtkOutlineFilter bounds
    bounds SetInput [reader GetOutput 1]

vtkPolyDataMapper boundsMapper
    boundsMapper SetInput [bounds GetOutput]

vtkActor boundsActor
    boundsActor SetMapper boundsMapper
    [boundsActor GetProperty] SetColor 0 0 0

vtkContourFilter contour
    contour SetInput [reader GetOutput 1]
    eval contour GenerateValues 5 0 .05


vtkPolyDataMapper contourMapper
    contourMapper SetInput [contour GetOutput]
    eval contourMapper SetScalarRange 0 .1
   [contourMapper GetLookupTable] SetHueRange 0.32 0

vtkActor contourActor
    contourActor SetMapper contourMapper
    [contourActor GetProperty] SetOpacity .5

# Create transfer mapping scalar value to opacity
vtkPiecewiseFunction opacityTransferFunction
    opacityTransferFunction AddPoint  0    0.01
    opacityTransferFunction AddPoint  255  0.35
    opacityTransferFunction ClampingOn

# Create transfer mapping scalar value to color
vtkColorTransferFunction colorTransferFunction
    colorTransferFunction AddHSVPoint    0.0 0.66 1.0 1.0
    colorTransferFunction AddHSVPoint   50.0 0.33 1.0 1.0
    colorTransferFunction AddHSVPoint  100.0 0.00 1.0 1.0

# The property describes how the data will look
vtkVolumeProperty volumeProperty
    volumeProperty SetColor colorTransferFunction
    volumeProperty SetScalarOpacity opacityTransferFunction
    volumeProperty SetInterpolationTypeToLinear

# The mapper / ray cast function know how to render the data
vtkVolumeRayCastCompositeFunction  compositeFunction

vtkVolumeRayCastMapper volumeMapper
#vtkVolumeTextureMapper2D volumeMapper
    volumeMapper SetVolumeRayCastFunction compositeFunction
    volumeMapper SetInput [readerSS GetOutput]

# The volume holds the mapper and the property and
# can be used to position/orient the volume

vtkVolume volume
    volume SetMapper volumeMapper
    volume SetProperty volumeProperty

ren1 AddVolume volume
#ren1 AddActor contourActor
ren1 AddActor boundsActor

######################################################################
vtkSphereSource Sphere
  Sphere SetCenter 0 0 0
  Sphere SetRadius 1
  Sphere SetThetaResolution 16
  Sphere SetStartTheta 0
  Sphere SetEndTheta 360
  Sphere SetPhiResolution 16
  Sphere SetStartPhi 0
  Sphere SetEndPhi 180

vtkGlyph3D Glyph
  Glyph SetInput [reader GetOutput]
  Glyph SelectInputVectors {}
  Glyph SetOrient 1
  Glyph SetColorMode 1
  #Glyph ScalingOn
  Glyph SelectInputScalars {}
  Glyph SetScaleMode 2
  Glyph SetScaleFactor .6
  Glyph SetSource [Sphere GetOutput]

vtkPolyDataMapper AtomsMapper
  AtomsMapper SetInput [Glyph GetOutput]
  AtomsMapper SetImmediateModeRendering 1
  AtomsMapper UseLookupTableScalarRangeOff
  AtomsMapper SetScalarVisibility 1
  AtomsMapper SetScalarModeToDefault

vtkActor Atoms
  Atoms SetMapper AtomsMapper
  Atoms SetUserTransform $xform
  [Atoms GetProperty] SetRepresentationToSurface
  [Atoms GetProperty] SetInterpolationToGouraud
  [Atoms GetProperty] SetAmbient 0.15
  [Atoms GetProperty] SetDiffuse 0.85
  [Atoms GetProperty] SetSpecular 0.1
  [Atoms GetProperty] SetSpecularPower 100
  [Atoms GetProperty] SetSpecularColor 1 1 1
  [Atoms GetProperty] SetColor 1 1 1

vtkTubeFilter Tube
  Tube SetInput [reader GetOutput]
  Tube SetNumberOfSides 16
  Tube SetCapping 0
  Tube SetRadius 0.2
  Tube SetVaryRadius 0
  Tube SetRadiusFactor 10

vtkPolyDataMapper BondsMapper
  BondsMapper SetInput [Tube GetOutput]
  BondsMapper SetImmediateModeRendering 1
  BondsMapper UseLookupTableScalarRangeOff
  BondsMapper SetScalarVisibility 1
  BondsMapper SetScalarModeToDefault
vtkActor Bonds
  Bonds SetMapper BondsMapper
  Bonds SetUserTransform $xform
  [Bonds GetProperty] SetRepresentationToSurface
  [Bonds GetProperty] SetInterpolationToGouraud
  [Bonds GetProperty] SetAmbient 0.15
  [Bonds GetProperty] SetDiffuse 0.85
  [Bonds GetProperty] SetSpecular 0.1
  [Bonds GetProperty] SetSpecularPower 100
  [Bonds GetProperty] SetSpecularColor 1 1 1
  [Bonds GetProperty] SetColor 1 1 1

ren1 AddActor Bonds
ren1 AddActor Atoms
####################################################

ren1 SetBackground 1 1 1

renWin Render

proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin AddObserver AbortCheckEvent {TkCheckAbort}

iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

wm withdraw .
