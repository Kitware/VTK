# Test the button source
package require vtk
package require vtkinteraction

# The image to map on the button
vtkJPEGReader r
  r SetFileName "$VTK_DATA_ROOT/Data/beach.jpg"
  r Update
vtkTexture t
  t SetInput [r GetOutput]
set dims [[r GetOutput] GetDimensions]
set d1 [lindex $dims 0]
set d2 [lindex $dims 1]

# The first button
vtkButtonSource bs
  bs SetWidth 2
  bs SetHeight 1
  bs SetDepth 0.2
  bs SetCircumferentialResolution 64
  bs SetRadialRatio 1.1
  bs SetShoulderResolution 8
  bs SetTextureResolution 4
  bs TwoSidedOn
vtkPolyDataMapper bMapper
  bMapper SetInput [bs GetOutput]
vtkActor b1
  b1 SetMapper bMapper
  b1 SetTexture t

# The second button
vtkButtonSource bs2
  bs2 SetWidth 2
  bs2 SetHeight 1
  bs2 SetDepth 0.2
  bs2 SetCircumferentialResolution 64
  bs2 SetRadialRatio 1.1
  bs2 SetShoulderResolution 8
  bs2 SetTextureResolution 4
  bs2 TwoSidedOn
  bs2 SetOrigin 2 0 0
  bs2 SetTextureStyleToFitImage
  bs2 SetTextureDimensions $d1 $d2
vtkPolyDataMapper b2Mapper
  b2Mapper SetInput [bs2 GetOutput]
vtkActor b2
  b2 SetMapper b2Mapper
  b2 SetTexture t

# Create the RenderWindow, Renderer and Interactive Renderer
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor b1
ren1 AddActor b2
ren1 SetBackground 0 0 0

renWin SetSize 300 100
renWin Render
[ren1 GetActiveCamera] Zoom 4.2
renWin Render

iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .
