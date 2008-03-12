package require vtk
package require vtkinteraction 

# Demonstrate how to use vtkAngleWidget to measure angles and distances between
# points.
#

# Start by reading in data.
#
vtkTIFFReader reader
  reader SetFileName "$VTK_DATA_ROOT/Data/beach.tif"

# "beach.tif" image contains ORIENTATION tag which is 
# ORIENTATION_TOPLEFT (row 0 top, col 0 lhs) type. The TIFF 
# reader parses this tag and sets the internal TIFF image 
# orientation accordingly.  To overwrite this orientation with a vtk
# convention of ORIENTATION_BOTLEFT (row 0 bottom, col 0 lhs ), invoke
# SetOrientationType method with parameter value of 4.
  reader SetOrientationType 4

# An actor to display the image.
#
vtkImageActor imageActor
  imageActor SetInput [reader GetOutput]

# Create a renderer and a render window,
#
vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1

# Create an interactor to respond to mouse events.
#
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

# Create an interactor style that works specifically with images:
# middle button: pan image
# right button: zoom image
# left button + ctrl key: rotate image
#
vtkInteractorStyleImage style
  iren SetInteractorStyle style

# VTK widgets consist of two parts: the widget part that handles event processing;
# and the widget representation that defines how the widget appears in the scene
# (i.e., matters pertaining to geometry).  The angle 2D representation consists
# of a pair of rays emanating from a common center point with an arc between
# the rays.  Text actors representing the length of the rays and the angle in
# degrees are placed midway along the various lines and updated in real time as
# the positions of the widget's handles changes.
#
# Create the widget and its representation.
#
vtkAngleRepresentation2D rep
  rep InstantiateHandleRepresentation

# Set the label format for the angle text
#
  rep SetLabelFormat "%-#7.3lf"

# Turn on lablelling of the length of the rays and set their numerical format.
#
  [ rep GetRay1 ] AutoLabelOn
  [ rep GetRay1 ] SetLabelFormat "%-#7.1lf"
  [ rep GetRay2 ] AutoLabelOn
  [ rep GetRay2 ] SetLabelFormat "%-#7.1lf"

# Do some additional formating of the text font.
#
  set textProp [[ rep GetRay1 ] GetLabelTextProperty ]
  $textProp SetColor 1 0 0
  $textProp ItalicOff
  $textProp ShadowOn
  [ rep GetRay2 ] SetLabelTextProperty $textProp
  [ rep GetArc  ] SetLabelTextProperty $textProp

# Set the color and thickness of the lines representing the rays and the arc.
#
  [[ rep GetRay1 ] GetProperty ] SetColor 0 0 1
  [[ rep GetRay1 ] GetProperty ] SetLineWidth 2
  [[ rep GetRay2 ] GetProperty ] SetColor 0 0 1
  [[ rep GetRay2 ] GetProperty ] SetLineWidth 2
  [[ rep GetArc ]  GetProperty ] SetColor 0 0 1
  [[ rep GetArc ]  GetProperty ] SetLineWidth 2

# Set the color of the handle representations.
# The handles are instantiated as vtkPointHandleRepresentation2D.
#
  [[ rep GetPoint1Representation ] GetProperty ] SetColor 1 0 0
  [[ rep GetPoint2Representation ] GetProperty ] SetColor 1 0 0
  [[ rep GetCenterRepresentation ] GetProperty ] SetColor 1 0 0

# Demonstrate how to set our own handles to be represented by circles instead of
# the default 2D cross cursor shape. Start by generating a circle from a
# cylinder having zero height.
#
vtkCylinderSource cylinder
  cylinder SetResolution 64
  cylinder SetRadius 6
  cylinder SetHeight 0.0
  cylinder CappingOff
  cylinder SetCenter 0 0 0

# The top and bottom of the cylinder still have separate points so merge them
#
vtkCleanPolyData clean
  clean PointMergingOn
  clean CreateDefaultLocator ""
  clean SetInputConnection 0 [ cylinder GetOutputPort 0 ]

# By default, VTK cylinders are created with their long axis aligned along the
# y-axis.  We will be performing angle measurements over an x-y plane, so
# rotate the new handle shape to be in this plane. First, create a rotation
# transform.
#
vtkTransform t
  t RotateX 90.0

# Create a filter to transform the polydata.
#
vtkTransformPolyDataFilter tpd
  tpd SetInputConnection 0  [ clean GetOutputPort 0 ]
  tpd SetTransform t
  tpd Update

# Apply the new cursor shape to the handle representations of the widget.
#
  [ rep GetPoint1Representation ] SetCursorShape [ tpd GetOutput ]
  [ rep GetPoint2Representation ] SetCursorShape [ tpd GetOutput ]
  [ rep GetCenterRepresentation ] SetCursorShape [ tpd GetOutput ]

# Create the actual widget that will listen and react to events.
#
vtkAngleWidget widget
  widget SetInteractor iren
  widget SetRepresentation rep

#  Add the actors to the renderer, set the background color and size.
#
  ren1 AddActor imageActor
  ren1 SetBackground 0.1 0.2 0.4
  renWin SetSize 600 600

# Render the image.
#
  iren AddObserver UserEvent {wm deiconify .vtkInteract}
  iren Initialize

# The widget starts out in an undefined state: the first mouse click defines
# the end of one of the rays, the second mouse click defines the location of
# the center, and the last mouse click defines the end of the other ray. Once
# defined, you can interact with the widget by moving the ray end points and
# the center point.  Pressing 'i' will toggle the widget off/on and the
# widget representation (i.e., point positions) will be preserved.
#
  widget On

# prevent the tk window from showing up then start the event loop
  wm withdraw .

