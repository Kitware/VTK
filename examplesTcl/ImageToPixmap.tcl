catch {load vtktcl}
# Test the new reader


source ../imaging/examplesTcl/vtkImageInclude.tcl


# Image pipeline

vtkImageFileReader reader
reader ReleaseDataFlagOff
reader SetDataMemoryOrder $VTK_IMAGE_COMPONENT_AXIS $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS 
reader SetFlips 0 0 1 0
reader SetDataDimensions 3 256 256
reader SetFileName "../../data/masonry.ppm"
reader SetDataScalarType $VTK_UNSIGNED_CHAR
#reader DebugOn

# Create the RenderWindow, Renderer and both Actors
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create a plane source and actor
vtkPlaneSource plane
vtkPolyDataMapper  planeMapper
planeMapper SetInput [plane GetOutput]
vtkActor planeActor
planeActor SetMapper planeMapper

# load in the texture map
#
vtkTexture atext
atext SetInput [reader GetOutput]
atext InterpolateOn
planeActor SetTexture atext

# Add the actors to the renderer, set the background and size
ren1 AddActor planeActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 500 500

# render the image
iren Initialize
set cam1 [ren1 GetActiveCamera]
$cam1 Elevation -30
$cam1 Roll -20
renWin Render

#renWin SetFileName "TPlane.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .







