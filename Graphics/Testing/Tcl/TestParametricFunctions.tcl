# ------------------------------------------------------------
# Purpose: Test the paraemtric functions.
# ------------------------------------------------------------

# ------------------------------------------------------------
# Call the VTK Tcl packages to make available all VTK commands
# ------------------------------------------------------------
package require vtk
package require vtkinteraction 

# ------------------------------------------------------------
# Get a texture
# ------------------------------------------------------------
vtkJPEGReader textureReader
  textureReader SetFileName "$VTK_DATA_ROOT/Data/beach.jpg"
vtkTexture texture
  texture SetInputConnection [textureReader GetOutputPort]

# ------------------------------------------------------------
# For each parametric surface:
# 1) Create it
# 2) Assign mappers and actors
# 3) Position ths object
# 5) Add a label
# ------------------------------------------------------------

# ------------------------------------------------------------
# Create a torus
# ------------------------------------------------------------
vtkParametricTorus torus
vtkParametricFunctionSource torusSource
  torusSource SetParametricFunction torus
  torusSource SetScalarModeToPhase

vtkPolyDataMapper torusMapper
  torusMapper SetInputConnection [torusSource GetOutputPort]
  torusMapper SetScalarRange 0 360

vtkActor torusActor
  torusActor SetMapper torusMapper
  torusActor SetPosition 0 12 0

vtkTextMapper torusTextMapper
    torusTextMapper SetInput "Torus"
    [torusTextMapper GetTextProperty] SetJustificationToCentered
    [torusTextMapper GetTextProperty] SetVerticalJustificationToCentered
    [torusTextMapper GetTextProperty] SetColor 1 0 0
    [torusTextMapper GetTextProperty] SetFontSize 14
vtkActor2D torusTextActor 
    torusTextActor SetMapper torusTextMapper    
    [torusTextActor GetPositionCoordinate] SetCoordinateSystemToWorld
    [torusTextActor GetPositionCoordinate] SetValue 0 9.5 0

# ------------------------------------------------------------
# Create a klein bottle
# ------------------------------------------------------------
vtkParametricKlein klein
vtkParametricFunctionSource kleinSource
  kleinSource SetParametricFunction klein
  kleinSource SetScalarModeToU0V0

vtkPolyDataMapper kleinMapper
  kleinMapper SetInputConnection [kleinSource GetOutputPort]
  kleinMapper SetScalarRange 0 3

vtkActor kleinActor
  kleinActor SetMapper kleinMapper
  kleinActor SetPosition 8 10.5 0

vtkTextMapper kleinTextMapper
    kleinTextMapper SetInput "Klein"
    [kleinTextMapper GetTextProperty] SetJustificationToCentered
    [kleinTextMapper GetTextProperty] SetVerticalJustificationToCentered
    [kleinTextMapper GetTextProperty] SetColor 1 0 0
    [kleinTextMapper GetTextProperty] SetFontSize 14
vtkActor2D kleinTextActor 
    kleinTextActor SetMapper kleinTextMapper    
    [kleinTextActor GetPositionCoordinate] SetCoordinateSystemToWorld
    [kleinTextActor GetPositionCoordinate] SetValue 8 9.5 0

# ------------------------------------------------------------
# Create a Figure-8 Klein
# ------------------------------------------------------------
vtkParametricFigure8Klein klein2
vtkParametricFunctionSource klein2Source
  klein2Source SetParametricFunction klein2
  klein2Source GenerateTextureCoordinatesOn

vtkPolyDataMapper klein2Mapper
  klein2Mapper SetInputConnection [klein2Source GetOutputPort]
  klein2Mapper SetScalarRange 0 3

vtkActor klein2Actor
  klein2Actor SetMapper klein2Mapper
  klein2Actor SetPosition 16 12 0
  klein2Actor SetTexture texture


vtkTextMapper fig8KleinTextMapper
    fig8KleinTextMapper SetInput "Fig-8 Klein"
    [fig8KleinTextMapper GetTextProperty] SetJustificationToCentered
    [fig8KleinTextMapper GetTextProperty] SetVerticalJustificationToCentered
    [fig8KleinTextMapper GetTextProperty] SetColor 1 0 0
    [fig8KleinTextMapper GetTextProperty] SetFontSize 14
vtkActor2D fig8KleinTextActor
    fig8KleinTextActor SetMapper fig8KleinTextMapper
    [fig8KleinTextActor GetPositionCoordinate] SetCoordinateSystemToWorld
    [fig8KleinTextActor GetPositionCoordinate] SetValue 16 9.5 0

# ------------------------------------------------------------
# Create a mobius strip
# ------------------------------------------------------------
vtkParametricMobius mobius
vtkParametricFunctionSource mobiusSource
  mobiusSource SetParametricFunction mobius
  mobiusSource GenerateTextureCoordinatesOn

vtkPolyDataMapper mobiusMapper
  mobiusMapper SetInputConnection [mobiusSource GetOutputPort]

vtkActor mobiusActor
  mobiusActor SetMapper mobiusMapper
  mobiusActor RotateX 45
  mobiusActor SetPosition 24 12 0
  mobiusActor SetTexture texture

vtkTextMapper mobiusTextMapper
    mobiusTextMapper SetInput "Mobius"
    [mobiusTextMapper GetTextProperty] SetJustificationToCentered
    [mobiusTextMapper GetTextProperty] SetVerticalJustificationToCentered
    [mobiusTextMapper GetTextProperty] SetColor 1 0 0
    [mobiusTextMapper GetTextProperty] SetFontSize 14
vtkActor2D mobiusTextActor
    mobiusTextActor SetMapper mobiusTextMapper
    [mobiusTextActor GetPositionCoordinate] SetCoordinateSystemToWorld
    [mobiusTextActor GetPositionCoordinate] SetValue 24 9.5 0

# ------------------------------------------------------------
# Create a super toroid
# ------------------------------------------------------------
vtkParametricSuperToroid toroid
toroid SetN1 2
toroid SetN2 3
vtkParametricFunctionSource toroidSource
  toroidSource SetParametricFunction toroid
  toroidSource SetScalarModeToU

vtkPolyDataMapper toroidMapper
  toroidMapper SetInputConnection [toroidSource GetOutputPort]
  toroidMapper SetScalarRange 0 6.28

vtkActor toroidActor
  toroidActor SetMapper toroidMapper
  toroidActor SetPosition 0 4 0

vtkTextMapper superToroidTextMapper
    superToroidTextMapper SetInput "Super Toroid"
    [superToroidTextMapper GetTextProperty] SetJustificationToCentered
    [superToroidTextMapper GetTextProperty] SetVerticalJustificationToCentered
    [superToroidTextMapper GetTextProperty] SetColor 1 0 0
    [superToroidTextMapper GetTextProperty] SetFontSize 14
vtkActor2D superToroidTextActor
    superToroidTextActor SetMapper superToroidTextMapper
    [superToroidTextActor GetPositionCoordinate] SetCoordinateSystemToWorld
    [superToroidTextActor GetPositionCoordinate] SetValue 0 1.5 0

# ------------------------------------------------------------
# Create a super ellipsoid
# ------------------------------------------------------------
vtkParametricSuperEllipsoid superEllipsoid
superEllipsoid SetXRadius 1.25
superEllipsoid SetYRadius 1.5
superEllipsoid SetZRadius 1.0
superEllipsoid SetN1 1.1
superEllipsoid SetN2 1.75
vtkParametricFunctionSource superEllipsoidSource
  superEllipsoidSource SetParametricFunction superEllipsoid
  superEllipsoidSource SetScalarModeToV

vtkPolyDataMapper superEllipsoidMapper
  superEllipsoidMapper SetInputConnection [superEllipsoidSource GetOutputPort]
  superEllipsoidMapper SetScalarRange 0 3.14

vtkActor superEllipsoidActor
  superEllipsoidActor SetMapper superEllipsoidMapper
  superEllipsoidActor SetPosition 8 4 0

vtkTextMapper superEllipsoidTextMapper
    superEllipsoidTextMapper SetInput "Super Ellipsoid"
    [superEllipsoidTextMapper GetTextProperty] SetJustificationToCentered
    [superEllipsoidTextMapper GetTextProperty] SetVerticalJustificationToCentered
    [superEllipsoidTextMapper GetTextProperty] SetColor 1 0 0
    [superEllipsoidTextMapper GetTextProperty] SetFontSize 14
vtkActor2D superEllipsoidTextActor
    superEllipsoidTextActor SetMapper superEllipsoidTextMapper
    [superEllipsoidTextActor GetPositionCoordinate] SetCoordinateSystemToWorld
    [superEllipsoidTextActor GetPositionCoordinate] SetValue 8 1.5 0

# ------------------------------------------------------------
# Create an open 1D spline
# ------------------------------------------------------------
vtkMath math
vtkPoints inputPoints
for {set i 0} {$i < 10} {incr i 1} {
    set x  [math Random -1 1]
    set y  [math Random -1 1]
    set z  [math Random -1 1]
    inputPoints InsertPoint $i $x $y $z
}
vtkParametricSpline spline
  spline SetPoints inputPoints
  spline ClosedOff
vtkParametricFunctionSource splineSource
  splineSource SetParametricFunction spline

vtkPolyDataMapper splineMapper
  splineMapper SetInputConnection [splineSource GetOutputPort]

vtkActor splineActor
  splineActor SetMapper splineMapper
  splineActor SetPosition 16 4 0
  [splineActor GetProperty] SetColor 0 0 0

vtkTextMapper splineTextMapper
    splineTextMapper SetInput "Open Spline"
    [splineTextMapper GetTextProperty] SetJustificationToCentered
    [splineTextMapper GetTextProperty] SetVerticalJustificationToCentered
    [splineTextMapper GetTextProperty] SetColor 1 0 0
    [splineTextMapper GetTextProperty] SetFontSize 14
vtkActor2D splineTextActor
    splineTextActor SetMapper splineTextMapper
    [splineTextActor GetPositionCoordinate] SetCoordinateSystemToWorld
    [splineTextActor GetPositionCoordinate] SetValue 16 1.5 0

# ------------------------------------------------------------
# Create a closed 1D spline
# ------------------------------------------------------------
vtkParametricSpline spline2
  spline2 SetPoints inputPoints
  spline2 ClosedOn
vtkParametricFunctionSource spline2Source
  spline2Source SetParametricFunction spline2

vtkPolyDataMapper spline2Mapper
  spline2Mapper SetInputConnection [spline2Source GetOutputPort]

vtkActor spline2Actor
  spline2Actor SetMapper spline2Mapper
  spline2Actor SetPosition 24 4 0
  [spline2Actor GetProperty] SetColor 0 0 0

vtkTextMapper spline2TextMapper
    spline2TextMapper SetInput "Closed Spline"
    [spline2TextMapper GetTextProperty] SetJustificationToCentered
    [spline2TextMapper GetTextProperty] SetVerticalJustificationToCentered
    [spline2TextMapper GetTextProperty] SetColor 1 0 0
    [spline2TextMapper GetTextProperty] SetFontSize 14
vtkActor2D spline2TextActor
    spline2TextActor SetMapper spline2TextMapper
    [spline2TextActor GetPositionCoordinate] SetCoordinateSystemToWorld
    [spline2TextActor GetPositionCoordinate] SetValue 24 1.5 0

# ------------------------------------------------------------
# Create a spiral conic
# ------------------------------------------------------------
vtkParametricConicSpiral sconic
  sconic SetA 0.8
  sconic SetB 2.5
  sconic SetC 0.4
vtkParametricFunctionSource sconicSource
  sconicSource SetParametricFunction sconic
  sconicSource SetScalarModeToDistance

vtkPolyDataMapper sconicMapper
  sconicMapper SetInputConnection [sconicSource GetOutputPort]
vtkActor sconicActor
  sconicActor SetMapper sconicMapper
  sconicMapper SetScalarRange 0 9
  sconicActor SetPosition 0 -4 0
  sconicActor SetScale 1.2 1.2 1.2

vtkTextMapper sconicTextMapper
    sconicTextMapper SetInput "Spiral Conic"
    [sconicTextMapper GetTextProperty] SetJustificationToCentered
    [sconicTextMapper GetTextProperty] SetVerticalJustificationToCentered
    [sconicTextMapper GetTextProperty] SetColor 1 0 0
    [sconicTextMapper GetTextProperty] SetFontSize 14
vtkActor2D sconicTextActor
    sconicTextActor SetMapper sconicTextMapper
    [sconicTextActor GetPositionCoordinate] SetCoordinateSystemToWorld
    [sconicTextActor GetPositionCoordinate] SetValue 0 -6.5 0

# ------------------------------------------------------------
# Create Boy's surface
# ------------------------------------------------------------
vtkParametricBoy boy
vtkParametricFunctionSource boySource
  boySource SetParametricFunction boy
  boySource SetScalarModeToModulus

vtkPolyDataMapper boyMapper
  boyMapper SetInputConnection [boySource GetOutputPort]
  boyMapper SetScalarRange 0 2
vtkActor boyActor
  boyActor SetMapper boyMapper
  boyActor SetPosition 8 -4 0
  boyActor SetScale 1.5 1.5 1.5

vtkTextMapper boyTextMapper
    boyTextMapper SetInput "Boy"
    [boyTextMapper GetTextProperty] SetJustificationToCentered
    [boyTextMapper GetTextProperty] SetVerticalJustificationToCentered
    [boyTextMapper GetTextProperty] SetColor 1 0 0
    [boyTextMapper GetTextProperty] SetFontSize 14
vtkActor2D boyTextActor
    boyTextActor SetMapper boyTextMapper
    [boyTextActor GetPositionCoordinate] SetCoordinateSystemToWorld
    [boyTextActor GetPositionCoordinate] SetValue 8 -6.5 0

# ------------------------------------------------------------
# Create a cross cap
# ------------------------------------------------------------
vtkParametricCrossCap crossCap
vtkParametricFunctionSource crossCapSource
  crossCapSource SetParametricFunction crossCap
  crossCapSource SetScalarModeToY

vtkPolyDataMapper crossCapMapper
  crossCapMapper SetInputConnection [crossCapSource GetOutputPort]
vtkActor crossCapActor
  crossCapActor SetMapper crossCapMapper
  crossCapActor RotateX 65
  crossCapActor SetPosition 16 -4 0
  crossCapActor SetScale 1.5 1.5 1.5

vtkTextMapper crossCapTextMapper
    crossCapTextMapper SetInput "Cross Cap"
    [crossCapTextMapper GetTextProperty] SetJustificationToCentered
    [crossCapTextMapper GetTextProperty] SetVerticalJustificationToCentered
    [crossCapTextMapper GetTextProperty] SetColor 1 0 0
    [crossCapTextMapper GetTextProperty] SetFontSize 14
vtkActor2D crossCapTextActor
    crossCapTextActor SetMapper crossCapTextMapper
    [crossCapTextActor GetPositionCoordinate] SetCoordinateSystemToWorld
    [crossCapTextActor GetPositionCoordinate] SetValue 16 -6.5 0

# ------------------------------------------------------------
# Create Dini's surface
# ------------------------------------------------------------
vtkParametricDini dini
vtkParametricFunctionSource diniSource
  diniSource SetScalarModeToDistance
  diniSource SetParametricFunction dini

vtkPolyDataMapper diniMapper
  diniMapper SetInputConnection [diniSource GetOutputPort]

vtkActor diniActor
  diniActor SetMapper diniMapper
  diniActor RotateX -90
  diniActor SetPosition 24 -3 0
  diniActor SetScale 1.5 1.5 0.5

vtkTextMapper diniTextMapper
    diniTextMapper SetInput "Dini"
    [diniTextMapper GetTextProperty] SetJustificationToCentered
    [diniTextMapper GetTextProperty] SetVerticalJustificationToCentered
    [diniTextMapper GetTextProperty] SetColor 1 0 0
    [diniTextMapper GetTextProperty] SetFontSize 14
vtkActor2D diniTextActor
    diniTextActor SetMapper diniTextMapper
    [diniTextActor GetPositionCoordinate] SetCoordinateSystemToWorld
    [diniTextActor GetPositionCoordinate] SetValue 24 -6.5 0

# ------------------------------------------------------------
# Create Enneper's surface
# ------------------------------------------------------------
vtkParametricEnneper enneper
vtkParametricFunctionSource enneperSource
  enneperSource SetParametricFunction enneper
  enneperSource SetScalarModeToQuadrant

vtkPolyDataMapper enneperMapper
  enneperMapper SetInputConnection [enneperSource GetOutputPort]
  enneperMapper SetScalarRange 1 4

vtkActor enneperActor
  enneperActor SetMapper enneperMapper
  enneperActor SetPosition 0 -12 0
  enneperActor SetScale 0.25 0.25 0.25

vtkTextMapper enneperTextMapper
    enneperTextMapper SetInput "Enneper"
    [enneperTextMapper GetTextProperty] SetJustificationToCentered
    [enneperTextMapper GetTextProperty] SetVerticalJustificationToCentered
    [enneperTextMapper GetTextProperty] SetColor 1 0 0
    [enneperTextMapper GetTextProperty] SetFontSize 14
vtkActor2D enneperTextActor 
    enneperTextActor SetMapper enneperTextMapper    
    [enneperTextActor GetPositionCoordinate] SetCoordinateSystemToWorld
    [enneperTextActor GetPositionCoordinate] SetValue 0 -14.5 0

# ------------------------------------------------------------
# Create an ellipsoidal surface
# ------------------------------------------------------------
vtkParametricEllipsoid ellipsoid
  ellipsoid SetXRadius 1
  ellipsoid SetYRadius 0.75
  ellipsoid SetZRadius 0.5
vtkParametricFunctionSource ellipsoidSource
  ellipsoidSource SetParametricFunction ellipsoid
  ellipsoidSource SetScalarModeToZ

vtkPolyDataMapper ellipsoidMapper
  ellipsoidMapper SetInputConnection [ellipsoidSource GetOutputPort]
  ellipsoidMapper SetScalarRange -0.5 0.5

vtkActor ellipsoidActor
  ellipsoidActor SetMapper ellipsoidMapper
  ellipsoidActor SetPosition 8 -12 0
  ellipsoidActor SetScale 1.5 1.5 1.5

vtkTextMapper ellipsoidTextMapper
    ellipsoidTextMapper SetInput "Ellipsoid"
    [ellipsoidTextMapper GetTextProperty] SetJustificationToCentered
    [ellipsoidTextMapper GetTextProperty] SetVerticalJustificationToCentered
    [ellipsoidTextMapper GetTextProperty] SetColor 1 0 0
    [ellipsoidTextMapper GetTextProperty] SetFontSize 14
vtkActor2D ellipsoidTextActor 
    ellipsoidTextActor SetMapper ellipsoidTextMapper    
    [ellipsoidTextActor GetPositionCoordinate] SetCoordinateSystemToWorld
    [ellipsoidTextActor GetPositionCoordinate] SetValue 8 -14.5 0

# ------------------------------------------------------------
# Create an surface with random hills on it.
# Note that for testing, we will disable the
# random generation of the surfaces. This is
# because random number generators do not
# return the same result on different operating
# systems.
# ------------------------------------------------------------
vtkParametricRandomHills randomHills
  randomHills AllowRandomGenerationOff
  randomHills GenerateTheHills
vtkParametricFunctionSource randomHillsSource
  randomHillsSource SetParametricFunction randomHills
  randomHillsSource GenerateTextureCoordinatesOn

vtkPolyDataMapper randomHillsMapper
  randomHillsMapper SetInputConnection [randomHillsSource GetOutputPort]

vtkActor randomHillsActor
  randomHillsActor SetMapper randomHillsMapper
  randomHillsActor SetPosition 16 -14 0
  randomHillsActor SetScale 0.2 0.2 0.2
  randomHillsActor SetTexture texture

vtkTextMapper randomHillsTextMapper
    randomHillsTextMapper SetInput "Random Hills"
    [randomHillsTextMapper GetTextProperty] SetJustificationToCentered
    [randomHillsTextMapper GetTextProperty] SetVerticalJustificationToCentered
    [randomHillsTextMapper GetTextProperty] SetColor 1 0 0
    [randomHillsTextMapper GetTextProperty] SetFontSize 14
vtkActor2D randomHillsTextActor 
    randomHillsTextActor SetMapper randomHillsTextMapper    
    [randomHillsTextActor GetPositionCoordinate] SetCoordinateSystemToWorld
    [randomHillsTextActor GetPositionCoordinate] SetValue 16 -14.5 0

# ------------------------------------------------------------
# Create an Steiner's Roman Surface.
# ------------------------------------------------------------
vtkParametricRoman roman
  roman SetRadius 1.5
vtkParametricFunctionSource romanSource
  romanSource SetParametricFunction roman
  romanSource SetScalarModeToX

vtkPolyDataMapper romanMapper
  romanMapper SetInputConnection [romanSource GetOutputPort]

vtkActor romanActor
  romanActor SetMapper romanMapper
  romanActor SetPosition 24 -12 0

vtkTextMapper romanTextMapper
    romanTextMapper SetInput "Roman"
    [romanTextMapper GetTextProperty] SetJustificationToCentered
    [romanTextMapper GetTextProperty] SetVerticalJustificationToCentered
    [romanTextMapper GetTextProperty] SetColor 1 0 0
    [romanTextMapper GetTextProperty] SetFontSize 14
vtkActor2D romanTextActor
    romanTextActor SetMapper romanTextMapper    
    [romanTextActor GetPositionCoordinate] SetCoordinateSystemToWorld
    [romanTextActor GetPositionCoordinate] SetValue 24 -14.5 0

# ------------------------------------------------------------
# Create the RenderWindow, Renderer and both Actors
# ------------------------------------------------------------
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# add actors
ren1 AddViewProp torusActor
ren1 AddViewProp kleinActor
ren1 AddViewProp klein2Actor
ren1 AddViewProp toroidActor
ren1 AddViewProp superEllipsoidActor
ren1 AddViewProp mobiusActor
ren1 AddViewProp splineActor
ren1 AddViewProp spline2Actor
ren1 AddViewProp sconicActor
ren1 AddViewProp boyActor
ren1 AddViewProp crossCapActor
ren1 AddViewProp diniActor
ren1 AddViewProp enneperActor
ren1 AddViewProp ellipsoidActor
ren1 AddViewProp randomHillsActor
ren1 AddViewProp romanActor
#add text actors
ren1 AddViewProp torusTextActor
ren1 AddViewProp kleinTextActor
ren1 AddViewProp fig8KleinTextActor
ren1 AddViewProp mobiusTextActor
ren1 AddViewProp superToroidTextActor
ren1 AddViewProp superEllipsoidTextActor
ren1 AddViewProp splineTextActor
ren1 AddViewProp spline2TextActor
ren1 AddViewProp sconicTextActor
ren1 AddViewProp boyTextActor
ren1 AddViewProp crossCapTextActor
ren1 AddViewProp diniTextActor
ren1 AddViewProp enneperTextActor
ren1 AddViewProp ellipsoidTextActor
ren1 AddViewProp randomHillsTextActor
ren1 AddViewProp romanTextActor

ren1 SetBackground 0.7 0.8 1
renWin SetSize 500 500
ren1 ResetCamera
[ren1 GetActiveCamera] Zoom 1.3
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .


