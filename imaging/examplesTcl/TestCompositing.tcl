catch {load vtktcl}
# demonstrate the use of 2D text

# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkTextMapper BlackMapper
  BlackMapper SetInput "Black"
  BlackMapper SetFontSize 18
  BlackMapper SetFontFamilyToArial

vtkActor2D BlackActor
  BlackActor SetMapper BlackMapper
  BlackActor SetPosition 0 0
  [BlackActor GetProperty] SetColor 0 0 1
  [BlackActor GetProperty] SetCompositingOperatorToBlack

vtkTextMapper NotDestMapper
  NotDestMapper SetInput "NotDest"
  NotDestMapper SetFontSize 18
  NotDestMapper SetFontFamilyToArial

vtkActor2D NotDestActor
  NotDestActor SetMapper NotDestMapper
  NotDestActor SetPosition 0 0
  [NotDestActor GetProperty] SetColor 0 0 1
  [NotDestActor GetProperty] SetCompositingOperatorToNotDest

vtkTextMapper SrcAndDestMapper
  SrcAndDestMapper SetInput "SrcAndDest"
  SrcAndDestMapper SetFontSize 18
  SrcAndDestMapper SetFontFamilyToArial

vtkActor2D SrcAndDestActor
  SrcAndDestActor SetMapper SrcAndDestMapper
  SrcAndDestActor SetPosition 0 0
  [SrcAndDestActor GetProperty] SetColor 0 0 1
  [SrcAndDestActor GetProperty] SetCompositingOperatorToSrcAndDest

vtkTextMapper SrcOrDestMapper
  SrcOrDestMapper SetInput "SrcOrDest"
  SrcOrDestMapper SetFontSize 18
  SrcOrDestMapper SetFontFamilyToArial

vtkActor2D SrcOrDestActor
  SrcOrDestActor SetMapper SrcOrDestMapper
  SrcOrDestActor SetPosition 0 0
  [SrcOrDestActor GetProperty] SetColor 0 0 1
  [SrcOrDestActor GetProperty] SetCompositingOperatorToSrcOrDest

vtkTextMapper NotSrcMapper
  NotSrcMapper SetInput "NotSrc"
  NotSrcMapper SetFontSize 18
  NotSrcMapper SetFontFamilyToArial

vtkActor2D NotSrcActor
  NotSrcActor SetMapper NotSrcMapper
  NotSrcActor SetPosition 0 0
  [NotSrcActor GetProperty] SetColor 0 0 1
  [NotSrcActor GetProperty] SetCompositingOperatorToNotSrc

vtkTextMapper SrcXorDestMapper
  SrcXorDestMapper SetInput "SrcXorDest"
  SrcXorDestMapper SetFontSize 18
  SrcXorDestMapper SetFontFamilyToArial

vtkActor2D SrcXorDestActor
  SrcXorDestActor SetMapper SrcXorDestMapper
  SrcXorDestActor SetPosition 0 0
  [SrcXorDestActor GetProperty] SetColor 0 0 1
  [SrcXorDestActor GetProperty] SetCompositingOperatorToSrcXorDest

vtkTextMapper SrcAndNotDestMapper
  SrcAndNotDestMapper SetInput "SrcAndNotDest"
  SrcAndNotDestMapper SetFontSize 18
  SrcAndNotDestMapper SetFontFamilyToArial

vtkActor2D SrcAndNotDestActor
  SrcAndNotDestActor SetMapper SrcAndNotDestMapper
  SrcAndNotDestActor SetPosition 0 0
  [SrcAndNotDestActor GetProperty] SetColor 0 0 1
  [SrcAndNotDestActor GetProperty] SetCompositingOperatorToSrcAndNotDest

vtkTextMapper SrcMapper
  SrcMapper SetInput "Src"
  SrcMapper SetFontSize 18
  SrcMapper SetFontFamilyToArial

vtkActor2D SrcActor
  SrcActor SetMapper SrcMapper
  SrcActor SetPosition 0 0
  [SrcActor GetProperty] SetColor 0 0 1
  [SrcActor GetProperty] SetCompositingOperatorToSrc

vtkTextMapper WhiteMapper
  WhiteMapper SetInput "White"
  WhiteMapper SetFontSize 18
  WhiteMapper SetFontFamilyToArial

vtkActor2D WhiteActor
  WhiteActor SetMapper WhiteMapper
  WhiteActor SetPosition 0 0
  [WhiteActor GetProperty] SetColor 0 0 1
  [WhiteActor GetProperty] SetCompositingOperatorToWhite

ren1 AddActor2D BlackActor
ren1 AddActor2D NotDestActor
ren1 AddActor2D SrcAndDestActor
ren1 AddActor2D SrcOrDestActor
ren1 AddActor2D NotSrcActor
ren1 AddActor2D SrcXorDestActor
ren1 AddActor2D SrcAndNotDestActor
ren1 AddActor2D SrcActor
ren1 AddActor2D WhiteActor

# throw in one Actor
vtkPlaneSource aPlane
vtkPolyDataMapper aMapper
  aMapper SetInput [aPlane GetOutput]
vtkActor anActor
  anActor SetMapper aMapper
  [anActor GetProperty] SetDiffuse 0
  [anActor GetProperty] SetAmbient 1
  [anActor GetProperty] SetColor 1 0 1
ren1 AddActor anActor

set y 162
BlackActor SetPosition 18 $y
incr y -18
NotDestActor SetPosition 18 $y
incr y -18
SrcAndDestActor SetPosition 18 $y
incr y -18
SrcOrDestActor SetPosition 18 $y
incr y -18
NotSrcActor SetPosition 18 $y
incr y -18
SrcXorDestActor SetPosition 18 $y
incr y -18
SrcAndNotDestActor SetPosition 18 $y
incr y -18
SrcActor SetPosition 18 $y
incr y -18
WhiteActor SetPosition 18 $y

ren1 SetBackground 1 1 1
renWin SetSize 160 200
[ren1 GetActiveCamera] Zoom 1.5
renWin Render

#renWin SetFileName "TestCompositing.tcl.ppm"
#renWin SaveImageAsPPM

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .



