package require vtk
package require vtkinteraction

# get the interactor ui

## Graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkImageGaussianSource gs1
  gs1 SetWholeExtent 0 31 0 31 0 31
  gs1 SetCenter 10 16 16
  gs1 SetMaximum 1000
  gs1 SetStandardDeviation 7

vtkImageGaussianSource gs2
  gs2 SetWholeExtent 0 31 0 31 0 31
  gs2 SetCenter 22 16 16
  gs2 SetMaximum 1000
  gs2 SetStandardDeviation 7

vtkImageAppendComponents iac
  iac AddInputConnection [gs1 GetOutputPort]
  iac AddInputConnection [gs2 GetOutputPort]

vtkContourFilter cf1
  cf1 SetInputConnection [iac GetOutputPort]
  cf1 SetValue 0 500
  cf1 SetArrayComponent 0

vtkContourFilter cf2
  cf2 SetInputConnection [iac GetOutputPort]
  cf2 SetValue 0 500
  cf2 SetArrayComponent 1

vtkPolyDataMapper mapper1
mapper1 SetInputConnection [cf1 GetOutputPort]
mapper1 SetImmediateModeRendering 1
mapper1 SetScalarRange 0 1
mapper1 SetScalarVisibility 0
mapper1 Update

vtkPolyDataMapper mapper2
mapper2 SetInputConnection [cf2 GetOutputPort]
mapper2 SetImmediateModeRendering 1
mapper2 SetScalarRange 0 1
mapper2 SetScalarVisibility 0

vtkActor actor1
actor1 SetMapper mapper1
[actor1 GetProperty] SetColor 1 1 1
ren1 AddActor actor1

vtkActor actor2
actor2 SetMapper mapper2
[actor2 GetProperty] SetColor 1 0 0
ren1 AddActor actor2

# Add the actors to the renderer, set the background and size
#
ren1 SetBackground .3 .3 .3
renWin SetSize 400 400

# enable user interface interactor
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .

