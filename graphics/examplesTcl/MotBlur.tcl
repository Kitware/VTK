catch {load vtktcl}
# this is a tcl version of the Mace example
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin



# create the piplinee, ball and spikes

  vtkSphereSource sphere
    sphere SetThetaResolution 7 ; sphere SetPhiResolution 7
  vtkPolyDataMapper sphereMapper
sphereMapper SetInput [sphere GetOutput]
  vtkActor sphereActor
    sphereActor SetMapper sphereMapper
  vtkActor sphereActor2
    sphereActor2 SetMapper sphereMapper
  vtkConeSource cone
    cone SetResolution 5

  vtkGlyph3D glyph
glyph SetInput [sphere GetOutput]
glyph SetSource [cone GetOutput]
    glyph SetVectorModeToUseNormal 
    glyph SetScaleModeToScaleByVector 
    glyph SetScaleFactor 0.25
  vtkPolyDataMapper spikeMapper
spikeMapper SetInput [glyph GetOutput]
  vtkActor spikeActor
    spikeActor SetMapper spikeMapper
  vtkActor spikeActor2
    spikeActor2 SetMapper spikeMapper

  spikeActor SetPosition 0 0.7 0
  sphereActor SetPosition 0 0.7 0
  spikeActor2 SetPosition 0 -0.7 0
  sphereActor2 SetPosition 0 -0.7 0

  ren1 AddActor sphereActor
  ren1 AddActor spikeActor
  ren1 AddActor sphereActor2
  ren1 AddActor spikeActor2
  ren1 SetBackground 0.1 0.2 0.4
  renWin SetSize 200 200
  renWin DoubleBufferOff 

# do the first render and then zoom in a little
  renWin Render 
[ren1 GetActiveCamera]   Zoom 1.5

  renWin SetSubFrames 21

for  {set i 0} {$i <= 1.0} {set i [expr $i + 0.05]} {
    spikeActor2 RotateY 2
    sphereActor2 RotateY 2
    renWin Render 
    }

iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

#renWin SetFileName MotBlur.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .

