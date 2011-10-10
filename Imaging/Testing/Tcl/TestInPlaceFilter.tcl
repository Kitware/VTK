package require vtk

# Derived from Cursor3D.  This script increases the coverage of the
# vtkImageInplaceFilter super class.



# global values
set CURSOR_X 20
set CURSOR_Y 20
set CURSOR_Z 20

set IMAGE_MAG_X 2
set IMAGE_MAG_Y 2
set IMAGE_MAG_Z 1



# pipeline stuff
vtkSLCReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/nut.slc"

# make the image a little biger
vtkImageMagnify magnify1
  magnify1 SetInputConnection [reader GetOutputPort]
magnify1 SetMagnificationFactors $IMAGE_MAG_X $IMAGE_MAG_Y $IMAGE_MAG_Z
  magnify1 ReleaseDataFlagOn

vtkImageMagnify magnify2
  magnify2 SetInputConnection [reader GetOutputPort]
  magnify2 SetMagnificationFactors $IMAGE_MAG_X $IMAGE_MAG_Y $IMAGE_MAG_Z
  magnify2 ReleaseDataFlagOn

# a filter that does in place processing (magnify ReleaseDataFlagOn)
vtkImageCursor3D cursor
cursor SetInputConnection [magnify1 GetOutputPort]
cursor SetCursorPosition [expr $CURSOR_X * $IMAGE_MAG_X] \
    [expr $CURSOR_Y * $IMAGE_MAG_Y] [expr $CURSOR_Z * $IMAGE_MAG_Z]
cursor SetCursorValue 255
cursor SetCursorRadius [expr 50 * $IMAGE_MAG_X]
# stream to increase coverage of in place filter.

# put thge two together in one image
vtkImageAppend imageAppend
imageAppend SetAppendAxis 0
imageAppend AddInputConnection [magnify2 GetOutputPort]
imageAppend AddInputConnection [cursor GetOutputPort]

vtkImageViewer viewer
viewer SetInputConnection [imageAppend GetOutputPort]
viewer SetZSlice [expr $CURSOR_Z * $IMAGE_MAG_Z]
viewer SetColorWindow 200
viewer SetColorLevel 80
#viewer DebugOn
viewer Render

viewer SetPosition 50 50

#make interface
viewer Render

