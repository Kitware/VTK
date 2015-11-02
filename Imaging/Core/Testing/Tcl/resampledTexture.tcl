package require vtk
package require vtkinteraction

# Demonstrate automatic resampling of textures (i.e., OpenGL only handles
# power of two texture maps. This examples exercise's vtk's automatic
# power of two resampling).
#

# get the interactor ui

# create pipeline
#
# generate texture map (not power of two)
vtkVolume16Reader v16
    v16 SetDataDimensions 64 64
    [v16 GetOutput] SetOrigin 0.0 0.0 0.0
    v16 SetDataByteOrderToLittleEndian
    v16 SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
    v16 SetImageRange 1 93
    v16 SetDataSpacing 3.2 3.2 1.5
vtkExtractVOI extract
    extract SetInputConnection [v16 GetOutputPort]
    extract SetVOI 32 32 0 63 0 92
vtkTexture atext
    atext SetInputConnection [extract GetOutputPort]
    atext InterpolateOn

# gnerate plane to map texture on to
vtkPlaneSource plane
    plane SetXResolution 1
    plane SetYResolution 1
vtkPolyDataMapper textureMapper
    textureMapper SetInputConnection [plane GetOutputPort]
vtkActor textureActor
    textureActor SetMapper textureMapper
    textureActor SetTexture atext

# Create the RenderWindow, Renderer
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor textureActor
renWin SetSize 250 250
ren1 SetBackground 0.1 0.2 0.4

iren Initialize


iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .


