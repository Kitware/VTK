# Test the button source
package require vtk
package require vtkinteraction

# The image to map on the button
vtkJPEGReader r
  r SetFileName "$VTK_DATA_ROOT/Data/beach.jpg"
  r Update
vtkTexture t
  t SetInputConnection [r GetOutputPort]
set dims [[r GetOutput] GetDimensions]
set d1 [lindex $dims 0]
set d2 [lindex $dims 1]

# The first elliptical button
vtkEllipticalButtonSource bs
  bs SetWidth 2
  bs SetHeight 1
  bs SetDepth 0.2
  bs SetCircumferentialResolution 64
  bs SetRadialRatio 1.1
  bs SetShoulderResolution 8
  bs SetTextureResolution 4
  bs TwoSidedOn
vtkPolyDataMapper bMapper
  bMapper SetInputConnection [bs GetOutputPort]
vtkActor b1
  b1 SetMapper bMapper
  b1 SetTexture t

# The second elliptical button
vtkEllipticalButtonSource bs2
  bs2 SetWidth 2
  bs2 SetHeight 1
  bs2 SetDepth 0.2
  bs2 SetCircumferentialResolution 64
  bs2 SetRadialRatio 1.1
  bs2 SetShoulderResolution 8
  bs2 SetTextureResolution 4
  bs2 TwoSidedOn
  bs2 SetCenter 2 0 0
  bs2 SetTextureStyleToFitImage
  bs2 SetTextureDimensions $d1 $d2
vtkPolyDataMapper b2Mapper
  b2Mapper SetInputConnection [bs2 GetOutputPort]
vtkActor b2
  b2 SetMapper b2Mapper
  b2 SetTexture t

# The third rectangular button
vtkRectangularButtonSource bs3
  bs3 SetWidth 1.5
  bs3 SetHeight 0.75
  bs3 SetDepth 0.2
  bs3 TwoSidedOn
  bs3 SetCenter 0 1 0
  bs3 SetTextureDimensions $d1 $d2
vtkPolyDataMapper b3Mapper
  b3Mapper SetInputConnection [bs3 GetOutputPort]
vtkActor b3
  b3 SetMapper b3Mapper
  b3 SetTexture t

# The fourth rectangular button
vtkRectangularButtonSource bs4
  bs4 SetWidth 1.5
  bs4 SetHeight 0.75
  bs4 SetDepth 0.2
  bs4 TwoSidedOn
  bs4 SetCenter 2 1 0
  bs4 SetTextureStyleToFitImage
  bs4 SetTextureDimensions $d1 $d2
vtkPolyDataMapper b4Mapper
  b4Mapper SetInputConnection [bs4 GetOutputPort]
vtkActor b4
  b4 SetMapper b4Mapper
  b4 SetTexture t

# Create the RenderWindow, Renderer and Interactive Renderer
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor b1
ren1 AddActor b2
ren1 AddActor b3
ren1 AddActor b4
ren1 SetBackground 0 0 0

renWin SetSize 250 150
renWin Render
[ren1 GetActiveCamera] Zoom 1.5
renWin Render

iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .
