catch {load vtktcl}
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/colors.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# read data
#
vtkOBJReader wavefront
  wavefront SetFileName ../../../vtkdata/Viewpoint/cow.obj
  wavefront Update

vtkConeSource cone
    cone SetResolution 6
    cone SetRadius .1

vtkTransform transform
    transform Translate 0.5 0.0 0.0

vtkTransformPolyDataFilter transformF
    transformF SetInput [cone GetOutput]
    transformF SetTransform transform

# we just clean the normals for efficiency (keep down number of cones)
vtkCleanPolyData clean
  clean SetInput [wavefront GetOutput]

vtkGlyph3D glyph
    glyph SetInput [clean GetOutput]
    glyph SetSource [transformF GetOutput]
    glyph SetVectorModeToUseNormal
    glyph SetScaleModeToScaleByVector
    glyph SetScaleFactor 0.4

vtkPolyDataMapper hairMapper
  hairMapper SetInput [glyph GetOutput]

vtkActor hair
  hair SetMapper hairMapper

vtkPolyDataMapper cowMapper
  cowMapper SetInput [wavefront GetOutput]

vtkActor cow
  cow SetMapper cowMapper


# Add the actors to the renderer, set the background and size
#
ren1 AddActor cow
ren1 AddActor hair
[ren1 GetActiveCamera] Dolly 2
[ren1 GetActiveCamera] Azimuth 30
[ren1 GetActiveCamera] Elevation 30

eval [hair GetProperty] SetDiffuseColor $saddle_brown
eval [cow GetProperty] SetDiffuseColor $beige

renWin SetSize 320 240
ren1 SetBackground .1 .2 .4

iren Initialize
renWin Render

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .

#renWin SetFileName OBJReader.tcl.ppm
#renWin SaveImageAsPPM


