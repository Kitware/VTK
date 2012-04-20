package require vtk

# Create a gaussian
vtkImageGaussianSource gs
gs SetWholeExtent 0 30 0 30 0 30
gs SetMaximum 255.0
gs SetStandardDeviation 5
gs SetCenter 15 15 15

# threshold to leave a gap that should show up for
# gradient opacity
vtkImageThreshold t
t SetInputConnection [gs GetOutputPort]
t ReplaceInOn
t SetInValue 0
t ThresholdBetween 150 200

# Use a shift scale to convert to unsigned char
vtkImageShiftScale ss
ss SetInputConnection [t GetOutputPort]
ss SetOutputScalarTypeToUnsignedChar

# grid will be used for two component dependent
vtkImageGridSource grid0
grid0 SetDataScalarTypeToUnsignedChar
grid0 SetGridSpacing 10 10 10
grid0 SetLineValue 200
grid0 SetFillValue 10
grid0 SetDataExtent 0 30 0 30 0 30

# use dilation to thicken the grid
vtkImageContinuousDilate3D d
d SetInputConnection [grid0 GetOutputPort]
d SetKernelSize 3 3 3

# Now make a two component dependent
vtkImageAppendComponents iac
iac AddInputConnection [d GetOutputPort]
iac AddInputConnection [ss GetOutputPort]

# Some more gaussians for the four component indepent case
vtkImageGaussianSource gs1
gs1 SetWholeExtent 0 30 0 30 0 30
gs1 SetMaximum 255.0
gs1 SetStandardDeviation 4
gs1 SetCenter 5 5 5

vtkImageThreshold t1
t1 SetInputConnection [gs1 GetOutputPort]
t1 ReplaceInOn
t1 SetInValue 0
t1 ThresholdBetween 150 256

vtkImageGaussianSource gs2
gs2 SetWholeExtent 0 30 0 30 0 30
gs2 SetMaximum 255.0
gs2 SetStandardDeviation 4
gs2 SetCenter 12 12 12

vtkImageGaussianSource gs3
gs3 SetWholeExtent 0 30 0 30 0 30
gs3 SetMaximum 255.0
gs3 SetStandardDeviation 4
gs3 SetCenter 19 19 19

vtkImageThreshold t3
t3 SetInputConnection [gs3 GetOutputPort]
t3 ReplaceInOn
t3 SetInValue 0
t3 ThresholdBetween 150 256

vtkImageGaussianSource gs4
gs4 SetWholeExtent 0 30 0 30 0 30
gs4 SetMaximum 255.0
gs4 SetStandardDeviation 4
gs4 SetCenter 26 26 26

# we need a few append filters ...
vtkImageAppendComponents iac1
iac1 AddInputConnection [t1 GetOutputPort]
iac1 AddInputConnection [gs2 GetOutputPort]

vtkImageAppendComponents iac2
iac2 AddInputConnection [iac1 GetOutputPort]
iac2 AddInputConnection [t3 GetOutputPort]

vtkImageAppendComponents iac3
iac3 AddInputConnection [iac2 GetOutputPort]
iac3 AddInputConnection [gs4 GetOutputPort]

# create the four component dependend -
# use lines in x, y, z for colors
vtkImageGridSource gridR
gridR SetDataScalarTypeToUnsignedChar
gridR SetGridSpacing 10 100 100
gridR SetLineValue 250
gridR SetFillValue 100
gridR SetDataExtent 0 30 0 30 0 30

vtkImageContinuousDilate3D dR
dR SetInputConnection [gridR GetOutputPort]
dR SetKernelSize 2 2 2

vtkImageGridSource gridG
gridG SetDataScalarTypeToUnsignedChar
gridG SetGridSpacing 100 10 100
gridG SetLineValue 250
gridG SetFillValue 100
gridG SetDataExtent 0 30 0 30 0 30

vtkImageContinuousDilate3D dG
dG SetInputConnection [gridG GetOutputPort]
dG SetKernelSize 2 2 2

vtkImageGridSource gridB
gridB SetDataScalarTypeToUnsignedChar
gridB SetGridSpacing 100 100 10
gridB SetLineValue   0
gridB SetFillValue 250
gridB SetDataExtent 0 30 0 30 0 30

vtkImageContinuousDilate3D dB
dB SetInputConnection [gridB GetOutputPort]
dB SetKernelSize 2 2 2

# need some appending
vtkImageAppendComponents iacRG
iacRG AddInputConnection [dR GetOutputPort]
iacRG AddInputConnection [dG GetOutputPort]

vtkImageAppendComponents iacRGB
iacRGB AddInputConnection [iacRG GetOutputPort]
iacRGB AddInputConnection [dB GetOutputPort]

vtkImageAppendComponents iacRGBA
iacRGBA AddInputConnection [iacRGB GetOutputPort]
iacRGBA AddInputConnection [ss GetOutputPort]

# We need a bunch of opacity functions

# this one is a simple ramp to .2
vtkPiecewiseFunction rampPoint2
rampPoint2 AddPoint   0 0.0
rampPoint2 AddPoint 255 0.2

# this one is a simple ramp to 1
vtkPiecewiseFunction ramp1
ramp1 AddPoint   0 0.0
ramp1 AddPoint 255 1.0

# this one shows a sharp surface
vtkPiecewiseFunction surface
surface AddPoint    0 0.0
surface AddPoint   10 0.0
surface AddPoint   50 1.0
surface AddPoint  255 1.0


# this one is constant 1
vtkPiecewiseFunction constant1
constant1 AddPoint   0 1.0
constant1 AddPoint 255 1.0

# this one is used for gradient opacity
vtkPiecewiseFunction gop
gop AddPoint   0 0.0
gop AddPoint  20 0.0
gop AddPoint  60 1.0
gop AddPoint 255 1.0


# We need a bunch of color functions

# This one is a simple rainbow
vtkColorTransferFunction rainbow
rainbow SetColorSpaceToHSV
rainbow HSVWrapOff
rainbow AddHSVPoint   0 0.1 1.0 1.0
rainbow AddHSVPoint 255 0.9 1.0 1.0

# this is constant red
vtkColorTransferFunction red
red AddRGBPoint   0 1 0 0
red AddRGBPoint 255 1 0 0

# this is constant green
vtkColorTransferFunction green
green AddRGBPoint   0 0 1 0
green AddRGBPoint 255 0 1 0

# this is constant blue
vtkColorTransferFunction blue
blue AddRGBPoint   0 0 0 1
blue AddRGBPoint 255 0 0 1

# this is constant yellow
vtkColorTransferFunction yellow
yellow AddRGBPoint   0 1 1 0
yellow AddRGBPoint 255 1 1 0


vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin SetSize 500 500
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

[ren1 GetCullers] InitTraversal
set culler [[ren1 GetCullers] GetNextItem]
$culler SetSortingStyleToBackToFront

# We need 25 mapper / actor pairs which we will render
# in a grid. Going down we will vary the input data
# with the top row unsigned char, then float, then
# two dependent components, then four dependent components
# then four independent components. Going across we
# will vary the rendering method with MIP, Composite,
# Composite Shade, Composite GO, and Composite GO Shade.

for { set j 0 } { $j < 5 } { incr j } {
   for { set i 0 } { $i < 5 } { incr i } {
      vtkVolumeProperty volumeProperty${i}${j}
      vtkFixedPointVolumeRayCastMapper volumeMapper${i}${j}
      volumeMapper${i}${j} SetSampleDistance 0.25

      vtkVolume volume${i}${j}
      volume${i}${j} SetMapper volumeMapper${i}${j}
      volume${i}${j} SetProperty volumeProperty${i}${j}

      volume${i}${j} AddPosition [expr $i*30] [expr $j*30] 0

      ren1 AddVolume volume${i}${j}
   }
}

for { set i 0 } { $i < 5 } { incr i } {
   volumeMapper0${i} SetInputConnection [t GetOutputPort]
   volumeMapper1${i} SetInputConnection [ss GetOutputPort]
   volumeMapper2${i} SetInputConnection [iac GetOutputPort]
   volumeMapper3${i} SetInputConnection [iac3 GetOutputPort]
   volumeMapper4${i} SetInputConnection [iacRGBA GetOutputPort]

   volumeMapper${i}0 SetBlendModeToMaximumIntensity
   volumeMapper${i}1 SetBlendModeToComposite
   volumeMapper${i}2 SetBlendModeToComposite
   volumeMapper${i}3 SetBlendModeToComposite
   volumeMapper${i}4 SetBlendModeToComposite

   volumeProperty0${i} IndependentComponentsOn
   volumeProperty1${i} IndependentComponentsOn
   volumeProperty2${i} IndependentComponentsOff
   volumeProperty3${i} IndependentComponentsOn
   volumeProperty4${i} IndependentComponentsOff

   volumeProperty0${i} SetColor rainbow
   volumeProperty0${i} SetScalarOpacity rampPoint2
   volumeProperty0${i} SetGradientOpacity constant1

   volumeProperty1${i} SetColor rainbow
   volumeProperty1${i} SetScalarOpacity rampPoint2
   volumeProperty1${i} SetGradientOpacity constant1

   volumeProperty2${i} SetColor rainbow
   volumeProperty2${i} SetScalarOpacity rampPoint2
   volumeProperty2${i} SetGradientOpacity constant1

   volumeProperty3${i} SetColor 0 red
   volumeProperty3${i} SetColor 1 green
   volumeProperty3${i} SetColor 2 blue
   volumeProperty3${i} SetColor 3 yellow
   volumeProperty3${i} SetScalarOpacity 0 rampPoint2
   volumeProperty3${i} SetScalarOpacity 1 rampPoint2
   volumeProperty3${i} SetScalarOpacity 2 rampPoint2
   volumeProperty3${i} SetScalarOpacity 3 rampPoint2
   volumeProperty3${i} SetGradientOpacity 0 constant1
   volumeProperty3${i} SetGradientOpacity 1 constant1
   volumeProperty3${i} SetGradientOpacity 2 constant1
   volumeProperty3${i} SetGradientOpacity 3 constant1
   volumeProperty3${i} SetComponentWeight 0 1
   volumeProperty3${i} SetComponentWeight 1 1
   volumeProperty3${i} SetComponentWeight 2 1
   volumeProperty3${i} SetComponentWeight 3 1

   volumeProperty4${i} SetColor rainbow
   volumeProperty4${i} SetScalarOpacity rampPoint2
   volumeProperty4${i} SetGradientOpacity constant1

   volumeProperty${i}2 ShadeOn
   volumeProperty${i}4 ShadeOn 0
   volumeProperty${i}4 ShadeOn 1
   volumeProperty${i}4 ShadeOn 2
   volumeProperty${i}4 ShadeOn 3
}

volumeProperty00 SetScalarOpacity ramp1
volumeProperty10 SetScalarOpacity ramp1
volumeProperty20 SetScalarOpacity ramp1
volumeProperty30 SetScalarOpacity 0 surface
volumeProperty30 SetScalarOpacity 1 surface
volumeProperty30 SetScalarOpacity 2 surface
volumeProperty30 SetScalarOpacity 3 surface
volumeProperty40 SetScalarOpacity ramp1

volumeProperty02 SetScalarOpacity surface
volumeProperty12 SetScalarOpacity surface
volumeProperty22 SetScalarOpacity surface
volumeProperty32 SetScalarOpacity 0 surface
volumeProperty32 SetScalarOpacity 1 surface
volumeProperty32 SetScalarOpacity 2 surface
volumeProperty32 SetScalarOpacity 3 surface
volumeProperty42 SetScalarOpacity surface

volumeProperty04 SetScalarOpacity surface
volumeProperty14 SetScalarOpacity surface
volumeProperty24 SetScalarOpacity surface
volumeProperty34 SetScalarOpacity 0 surface
volumeProperty34 SetScalarOpacity 1 surface
volumeProperty34 SetScalarOpacity 2 surface
volumeProperty34 SetScalarOpacity 3 surface
volumeProperty44 SetScalarOpacity surface

volumeProperty03 SetGradientOpacity gop
volumeProperty13 SetGradientOpacity gop
volumeProperty23 SetGradientOpacity gop
volumeProperty33 SetGradientOpacity 0 gop
volumeProperty33 SetGradientOpacity 2 gop
volumeProperty43 SetGradientOpacity gop

volumeProperty33 SetScalarOpacity 0 ramp1
volumeProperty33 SetScalarOpacity 2 ramp1

volumeProperty04 SetGradientOpacity gop
volumeProperty14 SetGradientOpacity gop
volumeProperty24 SetGradientOpacity gop
volumeProperty34 SetGradientOpacity 0 gop
volumeProperty34 SetGradientOpacity 2 gop
volumeProperty44 SetGradientOpacity gop

renWin Render

[ren1 GetActiveCamera] Dolly 1.3
[ren1 GetActiveCamera] Azimuth 15
[ren1 GetActiveCamera] Elevation 5
ren1 ResetCameraClippingRange

wm withdraw .

iren Initialize
