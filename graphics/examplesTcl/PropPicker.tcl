catch {load vtktcl}

# Demonstrate how to use picking (with vtkPropPicker) on a variety 
# of different subclasses (vtkActor, vtkLODActor, vtkActor2D,
# vtkScalarBarActor).
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# Create a bunch of different type of vtkProp and pick them
#
# a vtkActor - just a sphere
vtkSphereSource sphere2
    sphere2 SetThetaResolution 16
    sphere2 SetPhiResolution 16
    sphere2 SetCenter 1 1 1
vtkPolyDataMapper sphereMapper2
    sphereMapper2 SetInput [sphere2 GetOutput]
    sphereMapper2 GlobalImmediateModeRenderingOn
vtkActor sphereActor2
    sphereActor2 SetMapper sphereMapper2

# An image - some drawn rectangles
vtkImageCanvasSource2D image
    image SetNumberOfScalarComponents 3
    image SetScalarType 3 
    image SetExtent 0 120 0 120 0 0
    image SetDrawColor 100 100 0 1
    image FillBox 0 120 0 120
    image SetDrawColor 200 0 200 1
    image FillBox 32 100 50 75
vtkImageMapper imagemapper
    imagemapper SetInput image
    imagemapper SetColorWindow 25
    imagemapper SetColorLevel 1
vtkActor2D imageactor
    imageactor SetMapper imagemapper

# vtkLODActor (a mace)
vtkSphereSource sphere
vtkPolyDataMapper sphereMapper
    sphereMapper SetInput [sphere GetOutput]
    sphereMapper GlobalImmediateModeRenderingOn
vtkActor sphereActor
    sphereActor SetMapper sphereMapper
vtkConeSource cone
vtkGlyph3D glyph
    glyph SetInput [sphere GetOutput]
    glyph SetSource [cone GetOutput]
    glyph SetVectorModeToUseNormal
    glyph SetScaleModeToScaleByVector
    glyph SetScaleFactor 0.25
vtkPolyDataMapper spikeMapper
    spikeMapper SetInput [glyph GetOutput]
vtkLODActor spikeActor
    spikeActor SetMapper spikeMapper

# A text string rendered as a vtkActor2D
vtkTextMapper textMapper2
    textMapper2 SetFontFamilyToArial
    textMapper2 SetFontSize 10
    textMapper2 BoldOn
    textMapper2 SetInput "Any Old String"
vtkActor2D textActor2
    textActor2 VisibilityOn
    textActor2 SetMapper textMapper2
    [textActor2 GetPositionCoordinate] SetCoordinateSystemToNormalizedDisplay
    [textActor2 GetPositionCoordinate] SetValue .05 .75
    [textActor2 GetProperty] SetColor 0 1 0
    textActor2 SetPickMethod pickText 

# a vtkScalarBarActor
vtkScalarBarActor barActor
    barActor SetLookupTable [sphereMapper GetLookupTable]
    barActor SetOrientationToHorizontal
    [barActor GetProperty] SetColor 0 0 0
    [barActor GetPositionCoordinate] SetCoordinateSystemToNormalizedViewport
    [barActor GetPositionCoordinate] SetValue 0.1 0.01
    barActor SetOrientationToHorizontal
    barActor SetWidth 0.8
    barActor SetHeight 0.10
    barActor SetTitle "Cosine(<Velocity, PressureGradient>)"

# Classes that support picking - annotation text and the vtkPropPicker
vtkPropPicker picker
    picker SetEndPickMethod annotatePick
vtkTextMapper textMapper
    textMapper SetFontFamilyToArial
    textMapper SetFontSize 9
    textMapper BoldOn
vtkActor2D textActor
    textActor PickableOff
    textActor VisibilityOff
    textActor SetMapper textMapper
    [textActor GetProperty] SetColor 1 0 0

# Outlines are drawn around 3D objects
vtkOutlineSource outline
vtkPolyDataMapper mapper
vtkActor outlineActor
    outlineActor PickableOff
    outlineActor DragableOff
    mapper SetInput [outline GetOutput]
    outlineActor SetMapper mapper 
    [outlineActor GetProperty] SetColor 1 1 1
    outlineActor VisibilityOff

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin
    iren SetPicker picker

# Add the actors to the renderer, set the background and size
#
ren1 AddActor2D imageactor
ren1 AddActor2D textActor2
ren1 AddActor2D textActor
ren1 AddActor sphereActor2
ren1 AddActor sphereActor
ren1 AddActor spikeActor
ren1 AddActor barActor
ren1 AddActor outlineActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 410 400

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.4
iren Initialize
renWin SetFileName "PropPicker.tcl.ppm"
#renWin SaveImageAsPPM

proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin SetAbortCheckMethod {TkCheckAbort}

# prevent the tk window from showing up then start the event loop
wm withdraw .


### The following stuff is used to perform picking actions
proc unPick {} {
   [textActor2 GetProperty] SetColor 0 1 0
   textActor VisibilityOff
   outlineActor VisibilityOff
}

proc pickText {}  {
   [textActor2 GetProperty] SetColor 1 0 0
   outlineActor VisibilityOff
}
   
proc annotatePick {} {
   global picked
   set picked [picker GetProp] 
   set selPt [picker GetSelectionPoint]
   set x [lindex $selPt 0] 
   set y [lindex $selPt 1]
   # Clear all the pick displays
   unPick
   if { $picked == "" } {
       textMapper SetInput "nothing picked here..."
       textActor SetPosition $x $y
       textActor VisibilityOn 
    } else {
       set pickPos [picker GetPickPosition]
       set xp [lindex $pickPos 0] 
       set yp [lindex $pickPos 1]
       set zp [lindex $pickPos 2]
       
       # make sure picked object has a GetBounds 
       set methods [$picked ListMethods]
       set hasbounds 0
       foreach amethod $methods {
	  if { $amethod == "GetBounds"} {
	     set hasbounds 1
	     break
	  }
       }
       if { $hasbounds != 0 } {
	  set bounds [$picked GetBounds]
	  outline SetBounds [ lindex $bounds 0] [ lindex $bounds 1] \
                [ lindex $bounds 2] [ lindex $bounds 3] \
                [ lindex $bounds 4] [ lindex $bounds 5] 
	  outlineActor VisibilityOn
       } else {
	  outlineActor VisibilityOff
       }

       set name [$picked GetClassName]
       textMapper SetInput "$picked\n($name)"
       textActor SetPosition $x $y
       textActor VisibilityOn
    }

    renWin Render
}

# Create multiple output strings

renWin Render
renWin EraseOff
    picker PickProp  10 370 ren1
    picker PickProp 263 294 ren1
    picker PickProp 225 137 ren1
#    picker PickProp 239 170 ren1
    picker PickProp 117 306 ren1
    picker PickProp 290  38 ren1
    picker PickProp  16  85 ren1


