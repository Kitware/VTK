package require vtk
package require vtkinteraction

# Demonstrate how to use the vtkBoxWidget to control volume rendering within the 
# interior of the widget.

# Load a volume, use the manipulator to control what's volume rendered.
vtkVolume16Reader v16
  v16 SetDataDimensions 64 64
  [v16 GetOutput] SetOrigin 0.0 0.0 0.0
  v16 SetDataByteOrderToLittleEndian
  v16 SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
  v16 SetImageRange 1 93
  v16 SetDataSpacing 3.2 3.2 1.5

vtkPiecewiseFunction tfun
  tfun AddPoint   70.0  0.0
  tfun AddPoint  599.0  0
  tfun AddPoint  600.0 0
  tfun AddPoint  1195.0 0
  tfun AddPoint  1200 .2
  tfun AddPoint  1300 .3
  tfun AddPoint  2000 .3
  tfun AddPoint  4095.0  1.0

vtkColorTransferFunction ctfun
  ctfun AddRGBPoint      0.0 0.5  0.0  0.0
  ctfun AddRGBPoint    600.0 1.0  0.5  0.5
  ctfun AddRGBPoint   1280.0 0.9  0.2  0.3
  ctfun AddRGBPoint   1960.0 0.81 0.27 0.1
  ctfun AddRGBPoint   4095.0 0.5  0.5  0.5

vtkVolumeRayCastCompositeFunction  compositeFunction

vtkVolumeRayCastMapper volumeMapper
  volumeMapper SetInput [v16 GetOutput]
  volumeMapper SetVolumeRayCastFunction compositeFunction

vtkVolumeProperty volumeProperty
  volumeProperty SetColor ctfun
  volumeProperty SetScalarOpacity tfun
  volumeProperty SetInterpolationTypeToLinear
  volumeProperty ShadeOn

vtkVolume newvol
  newvol SetMapper volumeMapper
  newvol SetProperty volumeProperty

# Create an outline around the volume.
#
vtkOutlineFilter outline
    outline SetInput [v16 GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# The SetInteractor method is how 3D widgets are associated with the render
# window interactor. Internally, SetInteractor sets up a bunch of callbacks
# using the Command/Observer mechanism (AddObserver()).
vtkBoxWidget boxWidget
    boxWidget SetInteractor iren

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddVolume newvol

ren1 SetBackground 0 0 0
renWin SetSize 300 300

# place the interactor initially
boxWidget SetInput [v16 GetOutput]
boxWidget PlaceWidget
boxWidget InsideOutOn
boxWidget AddObserver StartInteractionEvent StartInteraction
boxWidget AddObserver InteractionEvent ClipVolumeRender
boxWidget AddObserver EndInteractionEvent EndInteraction
set outlineProperty [boxWidget GetOutlineProperty]
    $outlineProperty SetRepresentationToWireframe
    $outlineProperty SetAmbient 1.0
    $outlineProperty SetAmbientColor 1 1 1
    $outlineProperty SetLineWidth 3
set selectedOutlineProperty [boxWidget GetSelectedOutlineProperty]
    $selectedOutlineProperty SetRepresentationToWireframe
    $selectedOutlineProperty SetAmbient 1.0
    $selectedOutlineProperty SetAmbientColor 1 0 0
    $selectedOutlineProperty SetLineWidth 3

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .

proc StartInteraction {} {
   ren1 RemoveVolume newvol
}
proc EndInteraction {} {
   ren1 AddVolume newvol
   renWin Render
}

vtkPlanes planes
proc ClipVolumeRender {} {
   boxWidget GetPlanes planes
   planes Modified
   volumeMapper SetClippingPlanes planes
   renWin Render
}
