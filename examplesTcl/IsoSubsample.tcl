catch {load vtktcl}
# get the interactor ui
source vtkInt.tcl
source colors.tcl
source ../imaging/examplesTcl/vtkImageInclude.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create pipeline
#
vtkImageReader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
reader SetDataSpacing 0.8 0.8 1.5

vtkImageSubsample3D subsample
subsample SetInput [reader GetOutput]
subsample SetShrinkFactors 4 4 1

vtkImageGaussianSmooth smooth
smooth SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
smooth SetInput [reader GetOutput]
smooth SetStandardDeviations 1.75 1.75
smooth SetRadiusFactor 2
smooth SetStrides 4 4

vtkImageMarchingCubes iso
iso SetInput [subsample GetOutput]
# this is the anti aliased input
#iso SetInput [smooth GetOutput] 
iso SetValue 0 1150
iso SetInputMemoryLimit 140

vtkPolyDataMapper isoMapper
isoMapper SetInput [iso GetOutput]
isoMapper ScalarVisibilityOff

vtkActor isoActor
isoActor SetMapper isoMapper
eval [isoActor GetProperty] SetColor 0.92 0.92 0.92

# Add the actors to the renderer, set the background and size
#
ren1 AddActor isoActor
ren1 SetBackground 0.2 0.3 0.4
ren1 SetBackground 1 1 1
renWin SetSize 450 450
[ren1 GetActiveCamera] Elevation 97
[ren1 GetActiveCamera] SetViewUp 0 0 -1
[ren1 GetActiveCamera] Azimuth 165
[ren1 GetActiveCamera] Roll 5
[ren1 GetActiveCamera] Zoom 1.7
iren Initialize

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
#renWin SetFileName "mcubes.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


