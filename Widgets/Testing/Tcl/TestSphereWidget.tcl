package require vtk
package require vtkinteraction
package require vtktesting

# This example demonstrates how to use the vtkSphereWidget to control the
# position of a light.

# There are the pre-recorded events 
set Recording {
    "# StreamVersion 1
    CharEvent 23 266 0 0 105 1 i
    KeyReleaseEvent 23 266 0 0 105 1 i
    EnterEvent 69 294 0 0 0 0 i
    MouseMoveEvent 69 294 0 0 0 0 i
    MouseMoveEvent 68 293 0 0 0 0 i
    MouseMoveEvent 67 292 0 0 0 0 i
    MouseMoveEvent 66 289 0 0 0 0 i
    MouseMoveEvent 66 282 0 0 0 0 i
    MouseMoveEvent 66 271 0 0 0 0 i
    MouseMoveEvent 69 253 0 0 0 0 i
    MouseMoveEvent 71 236 0 0 0 0 i
    MouseMoveEvent 74 219 0 0 0 0 i
    MouseMoveEvent 76 208 0 0 0 0 i
    MouseMoveEvent 78 190 0 0 0 0 i
    MouseMoveEvent 78 173 0 0 0 0 i
    MouseMoveEvent 77 162 0 0 0 0 i
    MouseMoveEvent 77 151 0 0 0 0 i
    MouseMoveEvent 77 139 0 0 0 0 i
    MouseMoveEvent 76 125 0 0 0 0 i
    MouseMoveEvent 73 114 0 0 0 0 i
    MouseMoveEvent 73 106 0 0 0 0 i
    MouseMoveEvent 73 101 0 0 0 0 i
    MouseMoveEvent 72 95 0 0 0 0 i
    MouseMoveEvent 72 92 0 0 0 0 i
    MouseMoveEvent 70 89 0 0 0 0 i
    MouseMoveEvent 69 86 0 0 0 0 i
    MouseMoveEvent 67 84 0 0 0 0 i
    MouseMoveEvent 65 81 0 0 0 0 i
    MouseMoveEvent 60 79 0 0 0 0 i
    MouseMoveEvent 59 79 0 0 0 0 i
    MouseMoveEvent 58 79 0 0 0 0 i
    MouseMoveEvent 57 78 0 0 0 0 i
    MouseMoveEvent 55 78 0 0 0 0 i
    MouseMoveEvent 54 77 0 0 0 0 i
    LeftButtonPressEvent 54 77 0 0 0 0 i
    MouseMoveEvent 61 79 0 0 0 0 i
    MouseMoveEvent 67 83 0 0 0 0 i
    MouseMoveEvent 72 88 0 0 0 0 i
    MouseMoveEvent 77 90 0 0 0 0 i
    MouseMoveEvent 78 91 0 0 0 0 i
    MouseMoveEvent 80 92 0 0 0 0 i
    MouseMoveEvent 84 93 0 0 0 0 i
    MouseMoveEvent 85 94 0 0 0 0 i
    MouseMoveEvent 88 97 0 0 0 0 i
    MouseMoveEvent 90 100 0 0 0 0 i
    MouseMoveEvent 92 102 0 0 0 0 i
    MouseMoveEvent 94 103 0 0 0 0 i
    MouseMoveEvent 97 105 0 0 0 0 i
    MouseMoveEvent 101 107 0 0 0 0 i
    MouseMoveEvent 102 109 0 0 0 0 i
    MouseMoveEvent 104 111 0 0 0 0 i
    MouseMoveEvent 108 113 0 0 0 0 i
    MouseMoveEvent 112 115 0 0 0 0 i
    MouseMoveEvent 118 119 0 0 0 0 i
    MouseMoveEvent 118 120 0 0 0 0 i
    MouseMoveEvent 118 123 0 0 0 0 i
    MouseMoveEvent 120 125 0 0 0 0 i
    MouseMoveEvent 122 128 0 0 0 0 i
    MouseMoveEvent 123 129 0 0 0 0 i
    MouseMoveEvent 125 132 0 0 0 0 i
    MouseMoveEvent 125 134 0 0 0 0 i
    MouseMoveEvent 127 138 0 0 0 0 i
    MouseMoveEvent 127 142 0 0 0 0 i
    MouseMoveEvent 127 147 0 0 0 0 i
    MouseMoveEvent 126 152 0 0 0 0 i
    MouseMoveEvent 126 155 0 0 0 0 i
    MouseMoveEvent 125 160 0 0 0 0 i
    MouseMoveEvent 125 167 0 0 0 0 i
    MouseMoveEvent 125 169 0 0 0 0 i
    MouseMoveEvent 125 174 0 0 0 0 i
    MouseMoveEvent 122 179 0 0 0 0 i
    MouseMoveEvent 120 183 0 0 0 0 i
    MouseMoveEvent 116 187 0 0 0 0 i
    MouseMoveEvent 113 192 0 0 0 0 i
    MouseMoveEvent 113 193 0 0 0 0 i
    MouseMoveEvent 111 195 0 0 0 0 i
    MouseMoveEvent 108 198 0 0 0 0 i
    MouseMoveEvent 106 200 0 0 0 0 i
    MouseMoveEvent 104 202 0 0 0 0 i
    MouseMoveEvent 103 203 0 0 0 0 i
    MouseMoveEvent 99 205 0 0 0 0 i
    MouseMoveEvent 97 207 0 0 0 0 i
    MouseMoveEvent 94 208 0 0 0 0 i
    MouseMoveEvent 91 210 0 0 0 0 i
    MouseMoveEvent 89 211 0 0 0 0 i
    MouseMoveEvent 86 211 0 0 0 0 i
    MouseMoveEvent 84 211 0 0 0 0 i
    MouseMoveEvent 80 211 0 0 0 0 i
    MouseMoveEvent 77 211 0 0 0 0 i
    MouseMoveEvent 75 211 0 0 0 0 i
    MouseMoveEvent 71 211 0 0 0 0 i
    MouseMoveEvent 68 211 0 0 0 0 i
    MouseMoveEvent 66 210 0 0 0 0 i
    MouseMoveEvent 62 210 0 0 0 0 i
    MouseMoveEvent 58 209 0 0 0 0 i
    MouseMoveEvent 54 207 0 0 0 0 i
    MouseMoveEvent 52 204 0 0 0 0 i
    MouseMoveEvent 51 203 0 0 0 0 i
    MouseMoveEvent 51 200 0 0 0 0 i
    MouseMoveEvent 48 196 0 0 0 0 i
    MouseMoveEvent 45 187 0 0 0 0 i
    MouseMoveEvent 45 181 0 0 0 0 i
    MouseMoveEvent 44 168 0 0 0 0 i
    MouseMoveEvent 40 161 0 0 0 0 i
    MouseMoveEvent 39 154 0 0 0 0 i
    MouseMoveEvent 38 146 0 0 0 0 i
    MouseMoveEvent 35 131 0 0 0 0 i
    MouseMoveEvent 34 121 0 0 0 0 i
    MouseMoveEvent 34 110 0 0 0 0 i
    MouseMoveEvent 34 103 0 0 0 0 i
    MouseMoveEvent 34 91 0 0 0 0 i
    MouseMoveEvent 34 86 0 0 0 0 i
    MouseMoveEvent 34 73 0 0 0 0 i
    MouseMoveEvent 35 66 0 0 0 0 i
    MouseMoveEvent 37 60 0 0 0 0 i
    MouseMoveEvent 37 53 0 0 0 0 i
    MouseMoveEvent 38 50 0 0 0 0 i
    MouseMoveEvent 38 48 0 0 0 0 i
    MouseMoveEvent 41 45 0 0 0 0 i
    MouseMoveEvent 43 45 0 0 0 0 i
    MouseMoveEvent 44 45 0 0 0 0 i
    MouseMoveEvent 47 43 0 0 0 0 i
    MouseMoveEvent 51 44 0 0 0 0 i
    MouseMoveEvent 54 44 0 0 0 0 i
    MouseMoveEvent 55 44 0 0 0 0 i
    MouseMoveEvent 59 44 0 0 0 0 i
    MouseMoveEvent 64 44 0 0 0 0 i
    MouseMoveEvent 67 44 0 0 0 0 i
    MouseMoveEvent 68 44 0 0 0 0 i
    MouseMoveEvent 71 44 0 0 0 0 i
    MouseMoveEvent 74 44 0 0 0 0 i
    MouseMoveEvent 77 44 0 0 0 0 i
    MouseMoveEvent 80 45 0 0 0 0 i
    MouseMoveEvent 81 45 0 0 0 0 i
    MouseMoveEvent 85 49 0 0 0 0 i
    MouseMoveEvent 89 50 0 0 0 0 i
    MouseMoveEvent 94 52 0 0 0 0 i
    MouseMoveEvent 99 56 0 0 0 0 i
    MouseMoveEvent 104 58 0 0 0 0 i
    MouseMoveEvent 107 61 0 0 0 0 i
    MouseMoveEvent 109 63 0 0 0 0 i
    MouseMoveEvent 109 67 0 0 0 0 i
    MouseMoveEvent 111 83 0 0 0 0 i
    MouseMoveEvent 113 86 0 0 0 0 i
    MouseMoveEvent 113 87 0 0 0 0 i
    MouseMoveEvent 113 89 0 0 0 0 i
    MouseMoveEvent 112 93 0 0 0 0 i
    MouseMoveEvent 112 97 0 0 0 0 i
    MouseMoveEvent 111 104 0 0 0 0 i
    MouseMoveEvent 112 108 0 0 0 0 i
    MouseMoveEvent 116 115 0 0 0 0 i
    MouseMoveEvent 116 123 0 0 0 0 i
    MouseMoveEvent 116 129 0 0 0 0 i
    MouseMoveEvent 119 138 0 0 0 0 i
    MouseMoveEvent 122 141 0 0 0 0 i
    MouseMoveEvent 127 148 0 0 0 0 i
    MouseMoveEvent 128 161 0 0 0 0 i
    MouseMoveEvent 131 166 0 0 0 0 i
    MouseMoveEvent 134 168 0 0 0 0 i
    MouseMoveEvent 135 171 0 0 0 0 i
    MouseMoveEvent 134 174 0 0 0 0 i
    MouseMoveEvent 132 176 0 0 0 0 i
    MouseMoveEvent 132 178 0 0 0 0 i
    MouseMoveEvent 129 180 0 0 0 0 i
    MouseMoveEvent 127 182 0 0 0 0 i
    MouseMoveEvent 124 185 0 0 0 0 i
    MouseMoveEvent 122 186 0 0 0 0 i
    MouseMoveEvent 118 189 0 0 0 0 i
    MouseMoveEvent 114 191 0 0 0 0 i
    MouseMoveEvent 114 193 0 0 0 0 i
    MouseMoveEvent 112 193 0 0 0 0 i
    MouseMoveEvent 111 194 0 0 0 0 i
    MouseMoveEvent 110 197 0 0 0 0 i
    MouseMoveEvent 110 198 0 0 0 0 i
    MouseMoveEvent 109 199 0 0 0 0 i
    MouseMoveEvent 108 200 0 0 0 0 i
    MouseMoveEvent 108 201 0 0 0 0 i
    MouseMoveEvent 108 202 0 0 0 0 i
    MouseMoveEvent 108 203 0 0 0 0 i
    MouseMoveEvent 104 206 0 0 0 0 i
    LeftButtonReleaseEvent 104 206 0 0 0 0 i
    MouseMoveEvent 104 205 0 0 0 0 i
    MouseMoveEvent 104 204 0 0 0 0 i
    MouseMoveEvent 105 205 0 0 0 0 i
    MouseMoveEvent 105 206 0 0 0 0 i"
}

# Start by loading some data.
#
vtkDEMReader dem
    dem SetFileName "$VTK_DATA_ROOT/Data/SainteHelens.dem"
    dem Update

set Scale 2
vtkLookupTable lut
  lut SetHueRange 0.6 0
  lut SetSaturationRange 1.0 0
  lut SetValueRange 0.5 1.0
set lo [expr $Scale * [lindex [dem GetElevationBounds] 0]]
set hi [expr $Scale * [lindex [dem GetElevationBounds] 1]]

vtkImageShrink3D shrink
  shrink SetShrinkFactors 4 4 1
  shrink SetInputConnection [dem GetOutputPort]
  shrink AveragingOn

vtkImageDataGeometryFilter geom
  geom SetInputConnection [shrink GetOutputPort]
  geom ReleaseDataFlagOn

vtkWarpScalar warp
  warp SetInputConnection [geom GetOutputPort]
  warp SetNormal 0 0 1
  warp UseNormalOn
  warp SetScaleFactor $Scale
  warp ReleaseDataFlagOn

vtkElevationFilter elevation
  elevation SetInputConnection [warp GetOutputPort]
  elevation SetLowPoint 0 0 $lo
  elevation SetHighPoint 0 0 $hi
  eval elevation SetScalarRange $lo $hi
  elevation ReleaseDataFlagOn

vtkPolyDataNormals normals
  normals SetInput [elevation GetPolyDataOutput]
  normals SetFeatureAngle 60
  normals ConsistencyOff
  normals SplittingOff
  normals ReleaseDataFlagOn
  normals Update

vtkPolyDataMapper demMapper
  demMapper SetInputConnection [normals GetOutputPort]
  eval demMapper SetScalarRange $lo $hi
  demMapper SetLookupTable lut
  demMapper ImmediateModeRenderingOn

vtkActor demActor
  demActor SetMapper demMapper

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin
    iren LightFollowCameraOff
#    iren SetInteractorStyle ""

# Associate the line widget with the interactor
vtkSphereRepresentation rep
  rep SetPlaceFactor 4
  eval rep PlaceWidget [[normals GetOutput] GetBounds]
  rep HandleVisibilityOn
  rep SetRepresentationToWireframe
#  rep HandleVisibilityOff
#  rep HandleTextOff
vtkSphereWidget2 sphereWidget
  sphereWidget SetInteractor iren
  sphereWidget SetRepresentation rep
#  sphereWidget TranslationEnabledOff
#  sphereWidget ScalingEnabledOff
  sphereWidget AddObserver InteractionEvent MoveLight

vtkInteractorEventRecorder recorder
  recorder SetInteractor iren
#  recorder SetFileName "c:/record.log"
#  recorder Record
  recorder ReadFromInputStringOn
  recorder SetInputString $Recording

# Add the actors to the renderer, set the background and size
#
ren1 AddActor demActor
ren1 SetBackground 1 1 1
renWin SetSize 300 300
ren1 SetBackground 0.1 0.2 0.4

set cam1 [ren1 GetActiveCamera]
$cam1 SetViewUp 0 0 1
eval $cam1 SetFocalPoint [[dem GetOutput] GetCenter]
$cam1 SetPosition 1 0 0
ren1 ResetCamera
$cam1 Elevation 25
$cam1 Azimuth 125
$cam1 Zoom 1.25

vtkLight light
eval light SetFocalPoint [rep GetCenter]
eval light SetPosition [rep GetHandlePosition]
ren1 AddLight light

iren Initialize
renWin Render

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render

# Prevent the tk window from showing up then start the event loop.
wm withdraw .


# Actually probe the data

proc MoveLight {} {
    eval light SetPosition [rep GetHandlePosition]
}

recorder Play
