package require vtktcl

# Simple volume rendering example.
vtkSLCReader reader
    reader SetFileName "$VTK_DATA_ROOT/Data/sphere.slc"

# Create transfer functions for opacity and color
vtkPiecewiseFunction opacityTransferFunction
    opacityTransferFunction AddPoint  10   0.0
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
    volumeProperty ShadeOn

vtkVolumeRayCastCompositeFunction  compositeFunction

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin SetSize 600 300
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 SetBackground 0.1 0.2 0.4

for { set i 0 } { $i < 2 } { incr i } {
    for { set j 0 } { $j < 4 } { incr j } {

	vtkVolumeRayCastMapper volumeMapper_${i}_${j}
	volumeMapper_${i}_${j} SetInput [reader GetOutput]
	volumeMapper_${i}_${j} SetVolumeRayCastFunction compositeFunction
	volumeMapper_${i}_${j} SetSampleDistance 0.4
	volumeMapper_${i}_${j} CroppingOn
	volumeMapper_${i}_${j} SetCroppingRegionPlanes 17 33 17 33 17 33
	
	vtkVolume volume_${i}_${j}
	volume_${i}_${j} SetMapper volumeMapper_${i}_${j}
	volume_${i}_${j} SetProperty volumeProperty
	volume_${i}_${j} RotateX 30
	volume_${i}_${j} RotateY 30
	volume_${i}_${j} AddPosition [expr $j * 55] [expr $i * 55] 0

	ren1 AddProp volume_${i}_${j}
    }
}

volumeMapper_0_0 SetCroppingRegionFlagsToSubVolume
volumeMapper_0_1 SetCroppingRegionFlagsToCross
volumeMapper_0_2 SetCroppingRegionFlagsToInvertedCross
volumeMapper_0_3 SetCroppingRegionFlags 24600

volumeMapper_1_0 SetCroppingRegionFlagsToFence
volumeMapper_1_1 SetCroppingRegionFlagsToInvertedFence
volumeMapper_1_2 SetCroppingRegionFlags 1
volumeMapper_1_3 SetCroppingRegionFlags 67117057

[ren1 GetCullers] InitTraversal
set culler [[ren1 GetCullers] GetNextItem]
$culler SetSortingStyleToBackToFront

ren1 ResetCamera
[ren1 GetActiveCamera] Zoom 3.0
renWin Render


proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin SetAbortCheckMethod {TkCheckAbort}


iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

wm withdraw .



