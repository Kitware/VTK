catch {load vtktcl}
# A script to test the threshold filter.
# Values above 2000 are set to 255.
# Values below 2000 are set to 0.

source vtkImageInclude.tcl
source TkImageViewerInteractor.tcl


# Image pipeline

vtkImageCanvasSource2D canvas
canvas SetScalarType $VTK_UNSIGNED_CHAR
canvas SetExtent 0 339 0 339 0 0
# background black
canvas SetDrawColor 0
canvas FillBox 0 511 0 511

# thick box
canvas SetDrawColor 255
canvas FillBox 10 110 10 110
canvas SetDrawColor 0
canvas FillBox 30 90 30 90


# put a stop sign in the box
canvas SetDrawColor 255
canvas DrawSegment 52 80 68 80
canvas DrawSegment 68 80 80 68
canvas DrawSegment 80 68 80 52
canvas DrawSegment 80 52 68 40
canvas DrawSegment 68 40 52 40
canvas DrawSegment 52 40 40 52
canvas DrawSegment 40 52 40 68
canvas DrawSegment 40 68 52 80
canvas FillPixel 60 60

# diamond
canvas SetDrawColor 255
canvas FillTube 145 145 195 195 34
canvas SetDrawColor 0
canvas FillTube 165 165 175 175 7

# H
canvas SetDrawColor 255
canvas FillBox 230 250 230 330
canvas FillBox 310 330 230 330
canvas FillBox 230 330 270 290

# circle
canvas SetDrawColor 255
canvas DrawCircle 280 170  50.0

# point as center of circle
canvas SetDrawColor 255
canvas DrawPoint 280 170

# lines +
canvas DrawSegment 60 120 60 220 
canvas DrawSegment 10 170 110 170 

# lines X
canvas DrawSegment 10 230 110 330 
canvas DrawSegment 110 230 10 330 

# sloped lines
canvas DrawSegment 120 230 220 230 
canvas DrawSegment 120 230 220 250 
canvas DrawSegment 120 230 220 270 
canvas DrawSegment 120 230 220 290 
canvas DrawSegment 120 230 220 310 
canvas DrawSegment 120 230 220 330 
canvas DrawSegment 120 230 200 330 
canvas DrawSegment 120 230 180 330 
canvas DrawSegment 120 230 160 330 
canvas DrawSegment 120 230 140 330 
canvas DrawSegment 120 230 120 330 

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


vtkImageSkeleton2D skeleton1
#skeleton1 BypassOn
skeleton1 SetInput [canvas GetOutput]
skeleton1 SetPrune 0
skeleton1 SetNumberOfIterations 20
skeleton1 ReleaseDataFlagOff

vtkImageClip clip
clip SetInput [skeleton1 GetOutput]
clip SetOutputWholeExtent 0 120 0 120 0 0

vtkImageMagnify magnify
magnify SetInput [clip GetOutput]
magnify SetMagnificationFactors 5 5 1
magnify InterpolateOff
magnify ReleaseDataFlagOff

vtkImageViewer viewer1
viewer1 SetInput [canvas GetOutput]
viewer1 SetColorWindow 5
viewer1 SetColorLevel 1

vtkImageViewer viewer
#viewer SetInput [magnify GetOutput]
viewer SetInput [skeleton1  GetOutput]
viewer SetColorWindow 5
viewer SetColorLevel 1





# Create the GUI
wm withdraw .
toplevel .top 

frame .top.f1 
frame .top.f2

vtkTkImageViewerWidget .top.f1.r1 -width 340 -height 340 -iv viewer1
vtkTkImageViewerWidget .top.f1.r2 -width 340 -height 340 -iv viewer

button .top.btn  -text Quit -command exit

pack .top.f1.r1 .top.f1.r2 \
  -side left -padx 3 -pady 3 -expand t
pack .top.f1  -fill both -expand t
pack .top.btn -fill x


BindTkImageViewer .top.f1.r1 
BindTkImageViewer .top.f1.r2






