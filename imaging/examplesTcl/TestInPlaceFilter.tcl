catch {load vtktcl}
# Derived from Cursor3D.  This script increases the coverage of the
# vtkImageInplaceFilter super class.


source vtkImageInclude.tcl

# global values
set CURSOR_X 20
set CURSOR_Y 20
set CURSOR_Z 20

set IMAGE_MAG_X 2
set IMAGE_MAG_Y 2
set IMAGE_MAG_Z 1



# pipeline stuff
vtkSLCReader reader
    reader SetFileName "../../../vtkdata/poship.slc"

# make the image a little biger
vtkImageMagnify magnify
  magnify SetInput [reader GetOutput]
  magnify SetMagnificationFactors $IMAGE_MAG_X $IMAGE_MAG_Y $IMAGE_MAG_Z
  magnify ReleaseDataFlagOn

# one filter that just passes the data trough
vtkImageCursor3D cursor1
cursor1 SetInput [magnify GetOutput]
cursor1 SetCursorPosition [expr $CURSOR_X * $IMAGE_MAG_X] \
    [expr $CURSOR_Y * $IMAGE_MAG_Y] [expr $CURSOR_Z * $IMAGE_MAG_Z]
cursor1 SetCursorValue 255
cursor1 SetCursorRadius [expr 50 * $IMAGE_MAG_X]
cursor1 BypassOn

# a second filter that does in place processing (magnify ReleaseDataFlagOn)
vtkImageCursor3D cursor2
cursor2 SetInput [magnify GetOutput]
cursor2 SetCursorPosition [expr $CURSOR_X * $IMAGE_MAG_X] \
    [expr $CURSOR_Y * $IMAGE_MAG_Y] [expr $CURSOR_Z * $IMAGE_MAG_Z]
cursor2 SetCursorValue 255
cursor2 SetCursorRadius [expr 50 * $IMAGE_MAG_X]
# stream to increase coverage of in place filter.
[cursor2 GetInput] SetMemoryLimit 1

# put thge two together in one image
vtkImageAppend append
append SetAppendAxis 0
append AddInput [cursor1 GetOutput]
append AddInput [cursor2 GetOutput]

vtkImageViewer viewer
viewer SetInput [append GetOutput]
viewer SetZSlice [expr $CURSOR_Z * $IMAGE_MAG_Z]
viewer SetColorWindow 200
viewer SetColorLevel 80
#viewer DebugOn
viewer Render

viewer SetPosition 50 50

#make interface
source WindowLevelInterface.tcl

