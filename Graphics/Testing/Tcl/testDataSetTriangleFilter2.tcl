package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer, and RenderWindowInteractor
#
vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

# create pipeline
#
vtkDataSetReader reader
  reader SetFileName "$VTK_DATA_ROOT/Data/uGridEx.vtk"

vtkDataSetTriangleFilter tris
  tris SetInputConnection [reader GetOutputPort]

vtkShrinkFilter shrink
  shrink SetInputConnection [tris GetOutputPort]
  shrink SetShrinkFactor .8

vtkDataSetMapper mapper
  mapper SetInputConnection [shrink GetOutputPort]
  mapper SetScalarRange 0 26
vtkActor actor
  actor SetMapper mapper

# add the actor to the renderer; set the size
#
ren1 AddActor actor
renWin SetSize 350 350
ren1 SetBackground 1 1 1

[ren1 GetActiveCamera] SetPosition -4.01115 6.03964 10.5393 
[ren1 GetActiveCamera] SetFocalPoint 1 0.525 3.025 
[ren1 GetActiveCamera] SetViewAngle 30
[ren1 GetActiveCamera] SetViewUp 0.114284 0.835731 -0.537115 
[ren1 GetActiveCamera] SetClippingRange 4.83787 17.8392 

renWin Render

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .









