catch {load vtktcl}
# create selected cones
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/colors.tcl

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create pipeline
#
vtkStructuredPointsReader reader
    reader SetFileName "../../../vtkdata/carotid.vtk"
vtkThresholdPoints threshold
    threshold SetInput [reader GetOutput]
    threshold ThresholdByUpper 200
vtkMaskPoints mask
    mask SetInput [threshold GetOutput]
    mask SetOnRatio 10
vtkConeSource cone
    cone SetResolution 3
    cone SetHeight 1
    cone SetRadius 0.25
vtkGlyph3D cones
    cones SetInput [mask GetOutput]
    cones SetSource [cone GetOutput]
    cones SetScaleFactor 0.5
    cones SetScaleModeToScaleByVector
vtkLookupTable lut
    lut SetHueRange .667 0.0
    lut Build
vtkPolyDataMapper vecMapper
    vecMapper SetInput [cones GetOutput]
    vecMapper SetScalarRange 2 10
    vecMapper SetLookupTable lut
vtkActor vecActor
    vecActor SetMapper vecMapper

# contours of speed
vtkContourFilter iso
    iso SetInput [reader GetOutput]
    iso SetValue 0 190
vtkPolyDataMapper isoMapper
    isoMapper SetInput [iso GetOutput]
    isoMapper ScalarVisibilityOff
vtkActor isoActor
    isoActor SetMapper isoMapper
    [isoActor GetProperty] SetRepresentationToWireframe
    [isoActor GetProperty] SetOpacity 0.25

# outline
vtkOutlineFilter outline
    outline SetInput [reader GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
set outlineProp [outlineActor GetProperty]
eval $outlineProp SetColor 0 0 0

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor vecActor
ren1 AddActor isoActor
ren1 SetBackground 1 1 1
renWin SetSize 500 500
#renWin SetSize 1000 1000
ren1 SetBackground 0.1 0.2 0.4
[ren1 GetActiveCamera] Zoom 1.5
iren Initialize

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

#renWin SetFileName "thrshldV.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .
