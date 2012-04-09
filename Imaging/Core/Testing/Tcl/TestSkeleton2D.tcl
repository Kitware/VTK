package require vtk

# A script to test the threshold filter.
# Values above 2000 are set to 255.
# Values below 2000 are set to 0.

# Image pipeline

vtkImageCanvasSource2D imageCanvas
imageCanvas SetScalarTypeToUnsignedChar
imageCanvas SetExtent 0 339 0 339 0 0
# background black
imageCanvas SetDrawColor 0
imageCanvas FillBox 0 511 0 511

# thick box
imageCanvas SetDrawColor 255
imageCanvas FillBox 10 110 10 110
imageCanvas SetDrawColor 0
imageCanvas FillBox 30 90 30 90


# put a stop sign in the box
imageCanvas SetDrawColor 255
imageCanvas DrawSegment 52 80 68 80
imageCanvas DrawSegment 68 80 80 68
imageCanvas DrawSegment 80 68 80 52
imageCanvas DrawSegment 80 52 68 40
imageCanvas DrawSegment 68 40 52 40
imageCanvas DrawSegment 52 40 40 52
imageCanvas DrawSegment 40 52 40 68
imageCanvas DrawSegment 40 68 52 80
imageCanvas FillPixel 60 60

# diamond
imageCanvas SetDrawColor 255
imageCanvas FillTube 145 145 195 195 34
imageCanvas SetDrawColor 0
imageCanvas FillTube 165 165 175 175 7

# H
imageCanvas SetDrawColor 255
imageCanvas FillBox 230 250 230 330
imageCanvas FillBox 310 330 230 330
imageCanvas FillBox 230 330 270 290

# circle
imageCanvas SetDrawColor 255
imageCanvas DrawCircle 280 170  50.0

# point as center of circle
imageCanvas SetDrawColor 255
imageCanvas DrawPoint 280 170

# lines +
imageCanvas DrawSegment 60 120 60 220
imageCanvas DrawSegment 10 170 110 170

# lines X
imageCanvas DrawSegment 10 230 110 330
imageCanvas DrawSegment 110 230 10 330

# sloped lines
imageCanvas DrawSegment 120 230 220 230
imageCanvas DrawSegment 120 230 220 250
imageCanvas DrawSegment 120 230 220 270
imageCanvas DrawSegment 120 230 220 290
imageCanvas DrawSegment 120 230 220 310
imageCanvas DrawSegment 120 230 220 330
imageCanvas DrawSegment 120 230 200 330
imageCanvas DrawSegment 120 230 180 330
imageCanvas DrawSegment 120 230 160 330
imageCanvas DrawSegment 120 230 140 330
imageCanvas DrawSegment 120 230 120 330

# double thickness lines +
imageCanvas DrawSegment 120 60 220 60
imageCanvas DrawSegment 120 61 220 61
imageCanvas DrawSegment 170 10 170 110
imageCanvas DrawSegment 171 10 171 110

# lines X
imageCanvas DrawSegment 230 10 330 110
imageCanvas DrawSegment 231 10 331 110
imageCanvas DrawSegment 230 110 330 10
imageCanvas DrawSegment 231 110 331 10


vtkImageSkeleton2D skeleton1
#skeleton1 BypassOn
skeleton1 SetInputConnection [imageCanvas GetOutputPort]
skeleton1 SetPrune 0
skeleton1 SetNumberOfIterations 20
skeleton1 ReleaseDataFlagOff

vtkImageClip clip
clip SetInputConnection [skeleton1 GetOutputPort]
clip SetOutputWholeExtent 0 120 0 120 0 0

vtkImageMagnify magnify
magnify SetInputConnection [clip GetOutputPort]
magnify SetMagnificationFactors 5 5 1
magnify InterpolateOff
magnify ReleaseDataFlagOff

vtkImageViewer viewer1
viewer1 SetInputConnection [imageCanvas GetOutputPort]
viewer1 SetColorWindow 5
viewer1 SetColorLevel 1

vtkImageViewer viewer
#viewer SetInputConnection [magnify GetOutputPort]
viewer SetInputConnection [skeleton1  GetOutputPort]
viewer SetColorWindow 5
viewer SetColorLevel 1

viewer Render








