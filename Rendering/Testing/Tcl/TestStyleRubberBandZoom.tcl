package require vtk

# Set up the pipeline

vtkTIFFReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/beach.tif"

# "beach.tif" image contains ORIENTATION tag which is 
# ORIENTATION_TOPLEFT (row 0 top, col 0 lhs) type. The TIFF 
# reader parses this tag and sets the internal TIFF image 
# orientation accordingly.  To overwrite this orientation with a vtk
# convention of ORIENTATION_BOTLEFT (row 0 bottom, col 0 lhs ), invoke
# SetOrientationType method with parameter value of 4.
reader SetOrientationType 4

vtkImageActor ia
ia SetInput [reader GetOutput]

vtkRenderer ren
ren AddActor ia

vtkRenderWindow renWin
renWin AddRenderer ren
renWin SetSize 400 400

vtkRenderWindowInteractor iren
iren SetRenderWindow renWin

vtkInteractorStyleRubberBandZoom rbz
rbz SetInteractor iren

iren SetInteractorStyle rbz

renWin Render

# Test style

iren SetEventInformationFlipY 250 250 0 0 "0" 0 "0"
iren InvokeEvent "LeftButtonPressEvent"
iren SetEventInformationFlipY 100 100 0 0 "0" 0 "0"
iren InvokeEvent "MouseMoveEvent"
iren InvokeEvent "LeftButtonReleaseEvent"

wm withdraw .
