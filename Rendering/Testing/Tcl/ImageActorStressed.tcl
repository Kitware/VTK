package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and both Actors
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# First one tests the changing display extent without
# changing the size of the display extent (so it
# reuses a texture, but not a contiguous one)
vtkImageGaussianSource gsOne
gsOne SetWholeExtent 0 999 0 999 0 0
gsOne SetCenter 500 500 0
gsOne SetStandardDeviation 300
gsOne SetMaximum 255

vtkImageShiftScale ssOne
ssOne SetInput [gsOne GetOutput]
ssOne SetOutputScalarTypeToUnsignedChar
ssOne SetShift 0
ssOne SetScale 1
ssOne UpdateWholeExtent

vtkImageActor iaOne
iaOne SetInput [ssOne GetOutput]

ren1 AddActor iaOne

# The second one tests a really large texture
vtkImageGaussianSource gsTwo
gsTwo SetWholeExtent 1000 8999 1000 8999 0 0
gsTwo SetCenter 4000 4000 0
gsTwo SetStandardDeviation 2000
gsTwo SetMaximum 255

vtkImageShiftScale ssTwo
ssTwo SetInput [gsTwo GetOutput]
ssTwo SetOutputScalarTypeToUnsignedChar
ssTwo SetShift 0
ssTwo SetScale 1
ssTwo UpdateWholeExtent

vtkImageActor iaTwo
iaTwo SetInput [ssTwo GetOutput]
iaTwo SetScale 0.1 0.1 1.0
iaTwo AddPosition 1000 1000 0

ren1 AddActor iaTwo

# The third one will test changing input and a 
# power of two texture
vtkImageGaussianSource gsThree
gsThree SetWholeExtent 0 511 2000 2511 0 0
gsThree SetCenter 255 2255 0
gsThree SetStandardDeviation 100
gsThree SetMaximum 255

vtkImageShiftScale ssThree
ssThree SetInput [gsThree GetOutput]
ssThree SetOutputScalarTypeToUnsignedChar
ssThree SetShift 0
ssThree SetScale 1
ssThree UpdateWholeExtent

vtkImageActor iaThree
iaThree SetInput [ssThree GetOutput]

ren1 AddActor iaThree


# Same as first one, but the display extents
# represent contiguous section of memory that
# are powers of two
vtkImageGaussianSource gsFour
gsFour SetWholeExtent 2000 2511 0 511 0 0
gsFour SetCenter 2255 255 0
gsFour SetStandardDeviation 130
gsFour SetMaximum 255

vtkImageShiftScale ssFour
ssFour SetInput [gsFour GetOutput]
ssFour SetOutputScalarTypeToUnsignedChar
ssFour SetShift 0
ssFour SetScale 1
ssFour UpdateWholeExtent

vtkImageActor iaFour
iaFour SetInput [ssFour GetOutput]

ren1 AddActor iaFour

# Same as previous one, but the display extents
# represent contiguous section of memory that
# are not powers of two
vtkImageGaussianSource gsFive
gsFive SetWholeExtent 1200 1712 0 512 0 0
gsFive SetCenter 1456 256 0
gsFive SetStandardDeviation 130
gsFive SetMaximum 255

vtkImageShiftScale ssFive
ssFive SetInput [gsFive GetOutput]
ssFive SetOutputScalarTypeToUnsignedChar
ssFive SetShift 0
ssFive SetScale 1
ssFive UpdateWholeExtent

vtkImageActor iaFive
iaFive SetInput [ssFive GetOutput]

ren1 AddActor iaFive


ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 400 400

# render the image
iren AddObserver UserEvent {wm deiconify .vtkInteract}
[ren1 GetActiveCamera] Zoom 1.5
renWin Render


# prevent the tk window from showing up then start the event loop
wm withdraw .


renWin Render

iaOne SetDisplayExtent 200 999 200 999 0 0
iaFour SetDisplayExtent 2000 2511 0 300 0 0
iaFive SetDisplayExtent 1200 1712 0 300 0 0
gsThree SetStandardDeviation 120
renWin Render

iaOne SetDisplayExtent 0 799 0 799 0 0
iaFour SetDisplayExtent 2000 2511 200 500 0 0
iaFive SetDisplayExtent 1200 1712 200 500 0 0
gsThree SetStandardDeviation 150
renWin Render

