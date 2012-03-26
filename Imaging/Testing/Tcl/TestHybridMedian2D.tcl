# Show the constant kernel.  Smooth an impulse function.

package require vtk

vtkImageCanvasSource2D imageCanvas
imageCanvas SetScalarTypeToDouble
imageCanvas SetExtent 1 256 1 256 0 0
# back ground zero
imageCanvas SetDrawColor 0
imageCanvas FillBox 1 256 1 256

imageCanvas SetDrawColor 255
imageCanvas FillBox 30 225 30 225

imageCanvas SetDrawColor 0
imageCanvas FillBox 60 195 60 195

imageCanvas SetDrawColor 255
imageCanvas FillTube 100 100 154 154 40.0

imageCanvas SetDrawColor 0

imageCanvas DrawSegment 45 45 45 210
imageCanvas DrawSegment 45 210 210 210
imageCanvas DrawSegment 210 210 210 45
imageCanvas DrawSegment 210 45 45 45

imageCanvas DrawSegment 100 150 150 100
imageCanvas DrawSegment 110 160 160 110
imageCanvas DrawSegment 90 140 140 90
imageCanvas DrawSegment 120 170 170 120
imageCanvas DrawSegment 80 130 130 80
imageCanvas Update



set shotNoiseAmplitude 255.0
set shotNoiseFraction 0.1
#set shotNoiseExtent "1 256 1 256 0 0"

vtkImageNoiseSource shotNoiseSource
shotNoiseSource SetWholeExtent 1 256 1 256 0 0 ;#$shotNoiseExtent
shotNoiseSource SetMinimum 0.0
shotNoiseSource SetMaximum 1.0
shotNoiseSource ReleaseDataFlagOff

vtkImageThreshold shotNoiseThresh1
shotNoiseThresh1 SetInputConnection [shotNoiseSource GetOutputPort]
shotNoiseThresh1 ThresholdByLower [expr 1.0 - $shotNoiseFraction]
shotNoiseThresh1 SetInValue 0
shotNoiseThresh1 SetOutValue $shotNoiseAmplitude
shotNoiseThresh1 Update

vtkImageThreshold shotNoiseThresh2
shotNoiseThresh2 SetInputConnection [shotNoiseSource GetOutputPort]
shotNoiseThresh2 ThresholdByLower $shotNoiseFraction
shotNoiseThresh2 SetInValue [expr - $shotNoiseAmplitude]
shotNoiseThresh2 SetOutValue 0.0
shotNoiseThresh2 Update

vtkImageMathematics shotNoise
shotNoise SetInput1Data [shotNoiseThresh1 GetOutput]
shotNoise SetInput2Data [shotNoiseThresh2 GetOutput]
shotNoise SetOperationToAdd
shotNoise Update

vtkImageMathematics add
add SetInput1Data [shotNoise GetOutput]
add SetInput2Data [imageCanvas GetOutput]
add SetOperationToAdd





vtkImageMedian3D median
median SetInputConnection [add GetOutputPort]
median SetKernelSize 3 3 1

vtkImageHybridMedian2D hybrid1
hybrid1 SetInputConnection [add GetOutputPort]

vtkImageHybridMedian2D hybrid2
hybrid2 SetInputConnection [hybrid1 GetOutputPort]

vtkImageViewer viewer
viewer SetInputConnection [hybrid1 GetOutputPort]
viewer SetColorWindow 256
viewer SetColorLevel 127.5

viewer Render


