package require vtk
package require vtkinteraction

# Demonstrate how to use vtkCameraWidget to record and playback interpolated
# camera views.  This example also demonstrates the use of vtkSliderWidget, in
# this case to set the number of frames the camera widget will play back.
#
# Start by generating some data: the planet earth.
#
# Define a 2D plane to act as a base to apply a texture to.  The coordinates
# are specified as origin: {1, pi, 0}, point 1: {1, pi, 2*pi},
# point 2: {1, 0, 0} and can be thought of as a rectangle oriented parallel
# to the y-z plane and offset along the x-axis by 1.
#
vtkPlaneSource plane
  plane SetOrigin 1.0 [expr 3.14159265359 - 0.0001] 0.0
  plane SetPoint1 1.0 [expr 3.14159265359 - 0.0001] 6.28318530719
  plane SetPoint2 1.0 0.0001                        0.0
  plane SetXResolution 19
  plane SetYResolution 9

# Warp the plane's spherical coordinates to Cartesian coordinates using
# vtkSphericalTransfrom and vtkTransformPolyDataFilter.  The transform will
# map coordinates into a range suitable for texture mapping: [0,1].
#
vtkSphericalTransform transform

vtkTransformPolyDataFilter tpoly
  tpoly SetInputConnection [plane GetOutputPort]
  tpoly SetTransform transform

# Load an image: a map of the earth.
#
vtkPNMReader earth
  earth SetFileName "$VTK_DATA_ROOT/Data/earth.ppm"

# Apply the image to a vtkTexture so that the earth image can be mapped onto
# our sphere.
#
vtkTexture texture
  texture SetInputConnection [earth GetOutputPort]
  texture InterpolateOn

# Create a mapper to render the sphere.
#
vtkDataSetMapper mapper
  mapper SetInputConnection [tpoly GetOutputPort]

# Create an actor to display and interact with.  The actor will apply the
# texture map to geometry of the sphere.
#
vtkActor world
  world SetMapper mapper
  world SetTexture texture

# Create a renderer and a render window.
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1

# Create an interactor to respond to mouse events.
#
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin


# VTK widgets consist of two parts: the widget part that handles event processing;
# and the widget representation that defines how the widget appears in the scene
# (i.e., matters pertaining to geometry).  The camera representation inherits
# from vtkBorderRepresentation, a rectangular region, and is therefore
# essentially a 2D rectangle containing 3 pictograms: a camera that when clicked
# records the current camera position, a triangle to represent a playback button
# and an "X" to represent a reset button that erases all previously recorded
# camera positions.  The representation also sets the number of frames to
# generate during playback.
#
# Create the widget and its representation.
#

vtkCameraRepresentation cameraRep
  cameraRep SetNumberOfFrames 500

# An interpolator is used to fill in camera positions between the positions
# recorded by the widget.  Here we interpolate smoothly with a spline scheme.
#
vtkCameraInterpolator interpolator
  interpolator SetInterpolationTypeToSpline

  cameraRep SetInterpolator interpolator

# Position the rectangle to be in the lower left of the renderer window.
#
  [cameraRep GetPositionCoordinate] SetCoordinateSystemToNormalizedDisplay
  [cameraRep GetPositionCoordinate] SetValue 0.08 0.07
  [cameraRep GetPosition2Coordinate] SetCoordinateSystemToNormalizedDisplay
  [cameraRep GetPosition2Coordinate] SetValue 0.16 0.14

# Create the actual camera widget that will listen and react to events.
#
vtkCameraWidget cameraWidget
  cameraWidget SetInteractor iren
  cameraWidget SetRepresentation cameraRep
  cameraWidget KeyPressActivationOff

# Create a slider widget so that we can interactively set the number of frames
# that are interpolated between recorded camera positions.  First we create
# the widget's representation, in this case it will be a 2D slider.  A slider
# representation consists of 4 parts: a tube along which a slider runs and
# represents the range of values the slider can attain, the slider which has a
# text actor over top to display the current slider value, end caps
# to signify the ends of the slider's range and a text actor to display a
# title about what the slider represents (e.g., temperature, time, number
# of camera frames etc.).  The tube and the end caps can be clicked on and
# the slider will move to that chosen location.  The slider can be dragged along
# the tube.
#
  vtkSliderRepresentation2D sliderRep
  sliderRep SetMinimumValue 10
  sliderRep SetMaximumValue 1000
  sliderRep SetValue 500
  sliderRep SetTitleText "Number of Frames"

# Position the representation to be along the bottom of the render window
# and to the right of the camera widget.
#
  [sliderRep GetPoint1Coordinate] SetCoordinateSystemToNormalizedDisplay
  [sliderRep GetPoint1Coordinate] SetValue 0.3 0.1
  [sliderRep GetPoint2Coordinate] SetCoordinateSystemToNormalizedDisplay
  [sliderRep GetPoint2Coordinate] SetValue 0.9 0.1

# Specifiy the sizes of the slider, the end caps and the slider's tube.
#
  sliderRep SetSliderLength 0.02
  sliderRep SetSliderWidth 0.03
  sliderRep SetEndCapLength 0.01
  sliderRep SetEndCapWidth 0.03
  sliderRep SetTubeWidth 0.005
  sliderRep SetLabelFormat "%4.0lf"

# Create the actual slider widget that will listen and react to events.
#
vtkSliderWidget sliderWidget
  sliderWidget SetInteractor iren
  sliderWidget SetRepresentation sliderRep
  sliderWidget KeyPressActivationOff

# The slider widget can respond to interactions by either immediately updating
# the representation's slider position, or by animating it from its current
# position to a newly chosen one.
#
  sliderWidget SetAnimationModeToAnimate

# Add a callback to the slider widget to set the number of frames desired
# during playback by the camera widget.  More frames results in a longer/slower
# playback.
#
  sliderWidget AddObserver InteractionEvent callback

#  Add the actors to the renderer, set the background color and size.
#
  ren1 AddActor world
  ren1 SetBackground .1 .2 .4
  renWin SetSize 600 400

# Initialize the interactor so we can get the active camera.
#
  iren Initialize
  renWin Render

# Set the camera that the representation will record.
#
  cameraRep SetCamera [ ren1 GetActiveCamera ]

# Turn on both widgets.
#
  cameraWidget On
  sliderWidget On

  iren AddObserver UserEvent {wm deiconify .vtkInteract}

# Prevent the tk window from showing up then start the event loop.
#
  wm withdraw .

# vtkMath is needed to convert double values returned by the slider
# representation into integers.
#
  vtkMath math

# Callback for the interaction
#
proc callback { } {
  cameraRep SetNumberOfFrames [ eval math Round [ sliderRep GetValue ] ]
}
