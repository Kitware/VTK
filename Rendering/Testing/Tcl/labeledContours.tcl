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
    iso SetInput [v16 GetOutput]
    iso GenerateValues 6 500 1150
    iso Update
set numPts [[iso GetOutput] GetNumberOfPoints]
vtkPolyDataMapper isoMapper
    isoMapper SetInput [iso GetOutput]
    isoMapper ScalarVisibilityOn
    eval isoMapper SetScalarRange [[iso GetOutput] GetScalarRange]
vtkActor isoActor
    isoActor SetMapper isoMapper

# Subsample the points and label them
vtkMaskPoints mask
    mask SetInput [iso GetOutput]
    mask SetOnRatio [expr $numPts / 50]
    mask SetMaximumNumberOfPoints 50
    mask RandomModeOn

# Create labels for points - only show visible points
vtkSelectVisiblePoints visPts
    visPts SetInput [mask GetOutput]
    visPts SetRenderer ren1
vtkLabeledDataMapper ldm
    ldm SetInput [mask GetOutput]
    ldm SetLabelFormat "%g"
    ldm SetLabelModeToLabelScalars
    ldm SetFontFamilyToArial
    ldm SetFontSize 8
vtkActor2D contourLabels
    contourLabels SetMapper ldm
    contourLabels SetMapper ldm
    [contourLabels GetProperty] SetColor 1 0 0

# Add the actors to the renderer, set the background and size
#
ren1 AddActor2D isoActor
ren1 AddActor2D contourLabels

ren1 SetBackground 1 1 1
renWin SetSize 300 300
renWin Render
[ren1 GetActiveCamera] Zoom 1.5

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .

