catch {load vtktcl}
# A script to test the threshold filter.
# Values above 2000 are set to 255.
# Values below 2000 are set to 0.

source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 0 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
reader SetOutputScalarType $VTK_SHORT
#reader DebugOn

vtkImageThreshold thresh
thresh SetInput [reader GetOutput]
thresh SetOutputScalarType $VTK_UNSIGNED_CHAR
thresh ThresholdByUpper 2000.0
thresh SetInValue 255
thresh SetOutValue 0
thresh ReleaseDataFlagOff

vtkImageCanvasSource2D canvas
canvas SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_COMPONENT_AXIS 
canvas SetScalarType $VTK_UNSIGNED_CHAR
canvas SetExtent 0 511 0 511
# background black
canvas SetDrawColor 0
canvas FillBox 0 511 0 511

# thick box
canvas SetDrawColor 255
canvas FillBox 10 110 10 110
canvas SetDrawColor 0
canvas FillBox 30 90 30 90

# diamond
canvas SetDrawColor 255
canvas FillTube 145 145 195 195 35
canvas SetDrawColor 0
canvas FillTube 165 165 175 175 7

# H
canvas SetDrawColor 255
canvas FillBox 230 250 230 330
canvas FillBox 310 330 230 330
canvas FillBox 230 330 270 290

# circle
canvas SetDrawColor 255
canvas DrawCircle 390 390  50.0

# point as center of circle
canvas SetDrawColor 255
canvas DrawPoint 390 390

# lines +
canvas DrawSegment 60 120 60 220 
canvas DrawSegment 10 170 110 170 

# lines X
canvas DrawSegment 10 230 110 330 
canvas DrawSegment 110 230 10 330 

# sloped lines
canvas DrawSegment 10 340 110 340 
canvas DrawSegment 10 340 110 360 
canvas DrawSegment 10 340 110 380 
canvas DrawSegment 10 340 110 400 
canvas DrawSegment 10 340 110 420 
canvas DrawSegment 10 340 110 440 
canvas DrawSegment 10 340 90 440 
canvas DrawSegment 10 340 70 440 
canvas DrawSegment 10 340 50 440 
canvas DrawSegment 10 340 30 440 
canvas DrawSegment 10 340 10 440 

# double thickness lines +
canvas DrawSegment 120 60 220 60 
canvas DrawSegment 120 61 220 61 
canvas DrawSegment 170 10 170 110 
canvas DrawSegment 171 10 171 110 

# lines X
canvas DrawSegment 230 10 330 110
canvas DrawSegment 231 10 331 110
canvas DrawSegment 230 110 330 10 
canvas DrawSegment 231 110 331 10 

set prune 1

vtkImageSkeleton2D skeleton1
#skeleton1 BypassOn
skeleton1 SetInput [canvas GetOutput]
skeleton1 SetPrune $prune
skeleton1 ReleaseDataFlagOff

vtkImageSkeleton2D skeleton2
#skeleton1 BypassOn
skeleton2 SetInput [skeleton1 GetOutput]
skeleton2 SetPrune $prune
skeleton2 ReleaseDataFlagOff

vtkImageSkeleton2D skeleton3
#skeleton1 BypassOn
skeleton3 SetInput [skeleton2 GetOutput]
skeleton3 SetPrune $prune
skeleton3 ReleaseDataFlagOff

vtkImageSkeleton2D skeleton4
#skeleton1 BypassOn
skeleton4 SetInput [skeleton3 GetOutput]
skeleton4 SetPrune $prune
skeleton4 ReleaseDataFlagOff

vtkImageSkeleton2D skeleton5
#skeleton1 BypassOn
skeleton5 SetInput [skeleton4 GetOutput]
skeleton5 SetPrune $prune
skeleton5 ReleaseDataFlagOff

vtkImageSkeleton2D skeleton6
#skeleton1 BypassOn
skeleton6 SetInput [skeleton5 GetOutput]
skeleton6 SetPrune $prune
skeleton6 ReleaseDataFlagOff

vtkImageClip clip
clip SetInput [skeleton1 GetOutput]
clip SetOutputWholeExtent 0 120 0 120

vtkImageMagnify magnify
magnify SetInput [clip GetOutput]
magnify SetMagnificationFactors 5 5
magnify SetFilteredAxes $VTK_IMAGE_Y_AXIS $VTK_IMAGE_X_AXIS
magnify InterpolateOff
magnify ReleaseDataFlagOff

[skeleton1 GetOutput] DebugOn

vtkImageViewer viewer
viewer SetInput [magnify GetOutput]
#viewer SetInput [canvas GetOutput]
viewer SetInput [skeleton6  GetOutput]
viewer SetColorWindow 5
viewer SetColorLevel 1
#viewer DebugOn

# make interface
source WindowLevelInterface.tcl







