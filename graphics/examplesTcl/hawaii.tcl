catch {load vtktcl}
# this is a tcl version of hawaii coloration

# get the interactor ui
source vtkInt.tcl
source "colors.tcl"

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# read a vtk file
#
vtkPolyDataReader hawaii
    hawaii SetFileName "../../../data/honolulu.vtk"
    hawaii Update
vtkElevationFilter elevation
    elevation SetInput [hawaii GetOutput]
    elevation SetLowPoint 0 0 0
    elevation SetHighPoint 0 0 1000
    elevation SetScalarRange 0 1000
vtkLookupTable lut
lut SetHueRange 0.7 0
lut SetSaturationRange 1.0 0
lut SetValueRange 0.5 1.0
#    lut SetNumberOfColors 8
#    lut Build
#    eval lut SetTableValue 0 $turquoise_blue 1
#    eval lut SetTableValue 1 $sea_green_medium 1
#    eval lut SetTableValue 2 $sea_green_dark 1
#    eval lut SetTableValue 3 $olive_green_dark 1
#    eval lut SetTableValue 4 $brown 1
#    eval lut SetTableValue 5 $beige 1
#    eval lut SetTableValue 6 $light_beige 1
#    eval lut SetTableValue 7 $bisque 1
vtkDataSetMapper hawaiiMapper
    hawaiiMapper SetInput [elevation GetOutput]
    hawaiiMapper SetScalarRange 0 1000
    hawaiiMapper SetLookupTable lut
vtkActor hawaiiActor
    hawaiiActor SetMapper hawaiiMapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor hawaiiActor
ren1 SetBackground 1 1 1
renWin SetSize 500 500
ren1 SetBackground 0.1 0.2 0.4
renWin DoubleBufferOff

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

[ren1 GetActiveCamera] Zoom 1.8
renWin Render
#renWin SetFileName "hawaii.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


