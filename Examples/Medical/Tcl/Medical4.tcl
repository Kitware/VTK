package require vtk
package require vtkinteraction

#
# This example reads a volume dataset and displays it via volume rendering.
#

# Create the renderer, the render window, and the interactor. The renderer
# draws into the render window, the interactor enables mouse- and
# keyboard-based interaction with the scene.
vtkRenderer ren
vtkRenderWindow renWin
  renWin AddRenderer ren
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

# The following reader is used to read a series of 2D slices (images)
# that compose the volume. The slice dimensions are set, and the
# pixel spacing. The data Endianness must also be specified. The reader
# usese the FilePrefix in combination with the slice number to construct
# filenames using the format FilePrefix.%d. (In this case the FilePrefix
# is the root name of the file: quarter.)
vtkVolume16Reader v16
  v16 SetDataDimensions 64 64
  v16 SetImageRange 1 93
  v16 SetDataByteOrderToLittleEndian
  v16 SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
  v16 SetDataSpacing 3.2 3.2 1.5

# The volume will be displayed by ray-cast alpha compositing.
# A ray-cast mapper is needed to do the ray-casting, and a
# compositing function is needed to do the compositing along the ray.
vtkVolumeRayCastCompositeFunction rayCastFunction

vtkVolumeRayCastMapper volumeMapper
  volumeMapper SetInputConnection [v16 GetOutputPort]
  volumeMapper SetVolumeRayCastFunction rayCastFunction

# The color transfer function maps voxel intensities to colors.
# It is modality-specific, and often anatomy-specific as well.
# The goal is to one color for flesh (between 500 and 1000)
# and another color for bone (1150 and over).
vtkColorTransferFunction volumeColor
  volumeColor AddRGBPoint 0    0.0 0.0 0.0
  volumeColor AddRGBPoint 500  1.0 0.5 0.3
  volumeColor AddRGBPoint 1000 1.0 0.5 0.3
  volumeColor AddRGBPoint 1150 1.0 1.0 0.9

# The opacity transfer function is used to control the opacity
# of different tissue types.
vtkPiecewiseFunction volumeScalarOpacity
  volumeScalarOpacity AddPoint 0    0.00
  volumeScalarOpacity AddPoint 500  0.15
  volumeScalarOpacity AddPoint 1000 0.15
  volumeScalarOpacity AddPoint 1150 0.85

# The gradient opacity function is used to decrease the opacity
# in the "flat" regions of the volume while maintaining the opacity
# at the boundaries between tissue types.  The gradient is measured
# as the amount by which the intensity changes over unit distance.
# For most medical data, the unit distance is 1mm.
vtkPiecewiseFunction volumeGradientOpacity
  volumeGradientOpacity AddPoint 0   0.0
  volumeGradientOpacity AddPoint 90  0.5
  volumeGradientOpacity AddPoint 100 1.0

# The VolumeProperty attaches the color and opacity functions to the
# volume, and sets other volume properties.  The interpolation should
# be set to linear to do a high-quality rendering.  The ShadeOn option
# turns on directional lighting, which will usually enhance the
# appearance of the volume and make it look more "3D".  However,
# the quality of the shading depends on how accurately the gradient
# of the volume can be calculated, and for noisy data the gradient
# estimation will be very poor.  The impact of the shading can be
# decreased by increasing the Ambient coefficient while decreasing
# the Diffuse and Specular coefficient.  To increase the impact
# of shading, decrease the Ambient and increase the Diffuse and Specular.
vtkVolumeProperty volumeProperty
  volumeProperty SetColor volumeColor
  volumeProperty SetScalarOpacity volumeScalarOpacity
  volumeProperty SetGradientOpacity volumeGradientOpacity
  volumeProperty SetInterpolationTypeToLinear
  volumeProperty ShadeOn
  volumeProperty SetAmbient 0.4
  volumeProperty SetDiffuse 0.6
  volumeProperty SetSpecular 0.2

# The vtkVolume is a vtkProp3D (like a vtkActor) and controls the position
# and orientation of the volume in world coordinates.
vtkVolume volume
  volume SetMapper volumeMapper
  volume SetProperty volumeProperty

# Finally, add the volume to the renderer
ren AddViewProp volume

# Set up an initial view of the volume.  The focal point will be the
# center of the volume, and the camera position will be 400mm to the
# patient's left (whis is our right).
set camera [ren GetActiveCamera]
set c [volume GetCenter]
$camera SetFocalPoint [lindex $c 0] [lindex $c 1] [lindex $c 2]
$camera SetPosition [expr [lindex $c 0] + 400] [lindex $c 1] [lindex $c 2]
$camera SetViewUp 0 0 -1

# Increase the size of the render window
renWin SetSize 640 480

# Set up a callback (using command/observer) to bring up the Tcl
# command GUI when the keypress-u (UserEvent) key is pressed.
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

wm withdraw .
iren Start


