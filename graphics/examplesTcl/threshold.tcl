catch {load vtktcl}
# Generate marching cubes head model (full resolution)

# get the interactor ui and colors
source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/colors.tcl

# create pipeline
# reader reads slices
vtkVolume16Reader v16
    v16 SetDataDimensions 256 256
    v16 SetDataByteOrderToLittleEndian
    v16 SetFilePrefix "../../../vtkdata/fullHead/headsq"
    v16 SetDataSpacing 0.8 0.8 1.5
    v16 SetImageRange 30 50
    v16 SetDataMask 0x7fff

# extract thresholded cells
vtkThreshold threshold
  threshold SetInput [v16 GetOutput]
  threshold ThresholdBetween 400 700
  threshold AllScalarsOff

vtkContourFilter contour
  contour SetInput [threshold GetOutput]
  contour SetValue 0 600.5

#
vtkDataSetMapper mapper
  mapper SetInput [contour GetOutput]
  mapper ScalarVisibilityOff

vtkActor skin
    skin SetMapper mapper
    eval [skin GetProperty] SetColor $flesh

# Create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor skin
ren1 SetBackground 1 1 1
renWin SetSize 500 500
eval ren1 SetBackground $slate_grey
[ren1 GetActiveCamera] Zoom 1.5
[ren1 GetActiveCamera] Elevation 90

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

iren Initialize

#renWin SetFileName "threshold.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .
