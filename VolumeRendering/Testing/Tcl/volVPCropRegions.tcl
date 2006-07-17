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
    volumeProperty SetAmbient 0.1
    volumeProperty SetDiffuse 0.6
    volumeProperty SetSpecular 0.5
    volumeProperty SetSpecularPower 15.0

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin SetSize 400 400
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 SetBackground 0.1 0.2 0.4

vtkLight newLight
newLight SetPosition -20 100 0
newLight SetIntensity 0.5
newLight PositionalOn
newLight SetFocalPoint 78.669 52.6719 -0.473076

for { set i 0 } { $i < 2 } { incr i } {
    for { set j 0 } { $j < 3 } { incr j } {
       vtkVolumeProMapper volumeMapper_${i}_${j}
       volumeMapper_${i}_${j} SetInputConnection [reader GetOutputPort]
       volumeMapper_${i}_${j} SetBlendModeToComposite
       volumeMapper_${i}_${j} SuperSamplingOn
       volumeMapper_${i}_${j} SetSuperSamplingFactor 2.5 2.5 2.5
       volumeMapper_${i}_${j} CroppingOn
       volumeMapper_${i}_${j} SetCroppingRegionPlanes 17 33 17 33 17 33
       
       vtkVolume volume_${i}_${j}
       volume_${i}_${j} SetMapper volumeMapper_${i}_${j}
       volume_${i}_${j} SetProperty volumeProperty
       
       vtkTransform userMatrix_${i}_${j} 
       userMatrix_${i}_${j} PostMultiply
       userMatrix_${i}_${j} Identity
       userMatrix_${i}_${j} Translate -25 -25 -25

       if { $i == 0 } {
          userMatrix_${i}_${j} RotateX [expr $j*87 + 23]
          userMatrix_${i}_${j} RotateY 16
       } else {
          userMatrix_${i}_${j} RotateX 27
          userMatrix_${i}_${j} RotateY [expr $j*87 + 19]
       }
       
       userMatrix_${i}_${j} Translate [expr $j*55 + 25] [expr $i*55 + 25] 0

       volume_${i}_${j} SetUserTransform userMatrix_${i}_${j}

       ren1 AddViewProp volume_${i}_${j}
    }
}

volumeMapper_0_0 SetCroppingRegionFlagsToSubVolume
volumeMapper_0_1 SetCroppingRegionFlagsToCross
volumeMapper_0_2 SetCroppingRegionFlagsToInvertedCross

volumeMapper_1_0 SetCroppingRegionFlagsToFence
volumeMapper_1_1 SetCroppingRegionFlagsToInvertedFence

volumeMapper_1_2 CroppingOff
volumeMapper_1_2 SetSubVolume 17 33 17 33 17 33

[ren1 GetCullers] InitTraversal
set culler [[ren1 GetCullers] GetNextItem]
$culler SetSortingStyleToBackToFront

[ren1 GetActiveCamera] ParallelProjectionOn
ren1 ResetCamera
renWin Render

ren1 AddLight newLight

proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin AddObserver "AbortCheckEvent" {TkCheckAbort}


iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

wm withdraw .



