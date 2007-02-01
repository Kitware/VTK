package require vtk
package require vtkinteraction

# demonstrate labeling of contour with scalar value

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Read a slice and contour it
vtkVolume16Reader v16
    v16 SetDataDimensions 64 64
    [v16 GetOutput] SetOrigin 0.0 0.0 0.0
    v16 SetDataByteOrderToLittleEndian
    v16 SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
    v16 SetImageRange 45 45
    v16 SetDataSpacing 3.2 3.2 1.5
vtkContourFilter iso
    iso SetInputConnection [v16 GetOutputPort]
    iso GenerateValues 6 500 1150
    iso Update
set numPts [[iso GetOutput] GetNumberOfPoints]
vtkPolyDataMapper isoMapper
    isoMapper SetInputConnection [iso GetOutputPort]
    isoMapper ScalarVisibilityOn
    eval isoMapper SetScalarRange [[iso GetOutput] GetScalarRange]
vtkActor isoActor
    isoActor SetMapper isoMapper

# Subsample the points and label them
vtkMaskPoints mask
    mask SetInputConnection [iso GetOutputPort]
    mask SetOnRatio [expr $numPts / 50]
    mask SetMaximumNumberOfPoints 50
    mask RandomModeOn

# Create labels for points - only show visible points
vtkSelectVisiblePoints visPts
    visPts SetInputConnection [mask GetOutputPort]
    visPts SetRenderer ren1
vtkLabeledDataMapper ldm
    ldm SetInputConnection [mask GetOutputPort]
#    ldm SetLabelFormat "%g"
    ldm SetLabelModeToLabelScalars
set tprop [ldm GetLabelTextProperty]
    $tprop SetFontFamilyToArial
    $tprop SetFontSize 10
    $tprop SetColor 1 0 0
vtkActor2D contourLabels
    contourLabels SetMapper ldm

# Add the actors to the renderer, set the background and size
#
ren1 AddActor2D isoActor
ren1 AddActor2D contourLabels

ren1 SetBackground 1 1 1
renWin SetSize 500 500
renWin Render
[ren1 GetActiveCamera] Zoom 1.5

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .

