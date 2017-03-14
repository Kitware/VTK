package require vtk
package require vtkinteraction

vtkConeSource cone
cone SetHeight 3.0
cone SetRadius 1.0
cone SetResolution 10

vtkPolyDataMapper coneMapper
coneMapper SetInputConnection [cone GetOutputPort]

# Actor for opacity as a property value.
vtkActor coneActor
coneActor SetMapper coneMapper
[coneActor GetProperty] SetOpacity 0.5



# Actor for opacity through LUT.
vtkElevationFilter elevation
elevation SetInputConnection [cone GetOutputPort]

vtkPolyDataMapper coneMapper2
coneMapper2 SetInputConnection [elevation GetOutputPort]

vtkLookupTable lut
lut SetAlphaRange 0.9 0.1
lut SetHueRange 0 0
lut SetSaturationRange 1 1
lut SetValueRange 1 1

coneMapper2 SetLookupTable lut
coneMapper2 SetScalarModeToUsePointData
coneMapper2 SetScalarVisibility 1
coneMapper2 InterpolateScalarsBeforeMappingOn

vtkActor coneActorLUT
coneActorLUT SetMapper coneMapper2
coneActorLUT SetPosition 0.1 1.0 0
[coneActorLUT GetProperty] SetOpacity 0.99


# Actor for opacity through texture.
vtkPNGReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/alphachannel.png"
reader Update

vtkTexturedSphereSource sphere

vtkTexture texture
texture SetInputConnection [reader GetOutputPort]

vtkPolyDataMapper coneMapper3
coneMapper3 SetInputConnection [sphere GetOutputPort]

vtkActor coneActorTexture
coneActorTexture SetTexture texture
coneActorTexture SetMapper coneMapper3
coneActorTexture SetPosition 0 -1.0 0
[coneActorTexture GetProperty] SetColor 0.5 0.5 1
[coneActorTexture GetProperty] SetOpacity 0.99

vtkRenderer ren1
ren1 AddActor coneActor
ren1 AddActor coneActorLUT
ren1 AddActor coneActorTexture
ren1 SetBackground 0.1 0.2 0.4

ren1 SetUseDepthPeeling 1
# 20 layers of tranlucent geometry
ren1 SetMaximumNumberOfPeels 20
# 2 out of 1000 pixels
ren1 SetOcclusionRatio 0.002

vtkRenderWindow renWin
renWin SetMultiSamples 0
renWin SetAlphaBitPlanes 1
renWin AddRenderer ren1
renWin SetSize 300 300

vtkRenderWindowInteractor iren
iren SetRenderWindow renWin

vtkInteractorStyleTrackballCamera style
iren SetInteractorStyle style

iren AddObserver UserEvent {wm deiconify .vtkInteract}

iren Initialize

wm withdraw .

set camera [ren1 GetActiveCamera]
$camera SetPosition 9 -1 3
$camera SetViewAngle 30
$camera SetViewUp 0.05 0.96 0.24
$camera SetFocalPoint 0 0.25 0


ren1 ResetCameraClippingRange
renWin Render
puts [ren1 GetLastRenderingUsedDepthPeeling]
if { [ren1 GetLastRenderingUsedDepthPeeling] } {
    puts "depth peeling was used"
} else {
    puts "depth peeling was not used (alpha blending instead)"
}
