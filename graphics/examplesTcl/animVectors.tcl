catch {load vtktcl}
catch {load vtktcl}
# get the interactor ui
source vtkInt.tcl
source "colors.tcl"

# First create the render master
vtkRenderMaster rm

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow]
set ren1   [$renWin MakeRenderer]
set iren [$renWin MakeRenderWindowInteractor]

# read data
#
vtkStructuredPointsReader reader
    reader SetFileName "../../../data/carotid.vtk"
vtkThresholdPoints threshold
    threshold SetInput [reader GetOutput]
    threshold ThresholdByUpper 200

vtkLineSource line
    line SetResolution 1
vtkGlyph3D lines
    lines SetInput [threshold GetOutput]
    lines SetSource [line GetOutput]
    lines SetScaleFactor 0.005
    lines ScaleByScalar
    lines Update;#make range current
vtkPolyMapper vectorMapper
    vectorMapper SetInput [lines GetOutput]
    eval vectorMapper SetScalarRange [[lines GetOutput] GetScalarRange]
vtkActor vectorActor
    vectorActor SetMapper vectorMapper

# 8 texture maps
vtkStructuredPointsReader tmap1
  tmap1 SetFileName "../../../data/vecTex/vecAnim1.vtk"
vtkTexture texture1
  texture1 SetInput [tmap1 GetOutput]
  texture1 InterpolateOff
  texture1 RepeatOff

vtkStructuredPointsReader tmap2
  tmap2 SetFileName "../../../data/vecTex/vecAnim2.vtk"
vtkTexture texture2
  texture2 SetInput [tmap2 GetOutput]
  texture2 InterpolateOff
  texture2 RepeatOff

vtkStructuredPointsReader tmap3
  tmap3 SetFileName "../../../data/vecTex/vecAnim3.vtk"
vtkTexture texture3
  texture3 SetInput [tmap3 GetOutput]
  texture3 InterpolateOff
  texture3 RepeatOff

vtkStructuredPointsReader tmap4
  tmap4 SetFileName "../../../data/vecTex/vecAnim4.vtk"
vtkTexture texture4
  texture4 SetInput [tmap4 GetOutput]
  texture4 InterpolateOff
  texture4 RepeatOff

vtkStructuredPointsReader tmap5
  tmap5 SetFileName "../../../data/vecTex/vecAnim5.vtk"
vtkTexture texture5
  texture5 SetInput [tmap5 GetOutput]
  texture5 InterpolateOff
  texture5 RepeatOff

vtkStructuredPointsReader tmap6
  tmap6 SetFileName "../../../data/vecTex/vecAnim6.vtk"
vtkTexture texture6
  texture6 SetInput [tmap6 GetOutput]
  texture6 InterpolateOff
  texture6 RepeatOff

vtkStructuredPointsReader tmap7
  tmap7 SetFileName "../../../data/vecTex/vecAnim7.vtk"
vtkTexture texture7
  texture7 SetInput [tmap7 GetOutput]
  texture7 InterpolateOff
  texture7 RepeatOff

vtkStructuredPointsReader tmap8
  tmap8 SetFileName "../../../data/vecTex/vecAnim8.vtk"
vtkTexture texture8
  texture8 SetInput [tmap8 GetOutput]
  texture8 InterpolateOff
  texture8 RepeatOff

vectorActor SetTexture texture1

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors vectorActor
$ren1 SetBackground 1 1 1
$renWin SetSize 500 500

$iren Initialize

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract}
[$ren1 GetActiveCamera] Zoom 1.5
$renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .

# go into loop

for {set i 0} {$i<5} {incr i} {
    vectorActor SetTexture texture1; $renWin Render
    vectorActor SetTexture texture2; $renWin Render
    vectorActor SetTexture texture3; $renWin Render
    vectorActor SetTexture texture4; $renWin Render
    vectorActor SetTexture texture5; $renWin Render
    vectorActor SetTexture texture6; $renWin Render
    vectorActor SetTexture texture7; $renWin Render
    vectorActor SetTexture texture8; $renWin Render
    vectorActor SetTexture texture1; $renWin Render
    vectorActor SetTexture texture2; $renWin Render
    vectorActor SetTexture texture3; $renWin Render
    vectorActor SetTexture texture4; $renWin Render
    vectorActor SetTexture texture5; $renWin Render
    vectorActor SetTexture texture6; $renWin Render
    vectorActor SetTexture texture7; $renWin Render
    vectorActor SetTexture texture8; $renWin Render
} 

#$renWin SetFileName animVectors.tcl.ppm
#$renWin SaveImageAsPPM
