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




set shotNoiseAmplitude 255.0
set shotNoiseFraction 0.1
set shotNoiseExtent "1 256 1 256 0 0"

vtkImageNoiseSource shotNoiseSource
eval shotNoiseSource SetWholeExtent $shotNoiseExtent
shotNoiseSource SetMinimum 0.0
shotNoiseSource SetMaximum 1.0
shotNoiseSource ReleaseDataFlagOff

vtkImageThreshold shotNoiseThresh1
shotNoiseThresh1 SetInput [shotNoiseSource GetOutput]
shotNoiseThresh1 ThresholdByLower [expr 1.0 - $shotNoiseFraction]
shotNoiseThresh1 SetInValue 0
shotNoiseThresh1 SetOutValue $shotNoiseAmplitude

vtkImageThreshold shotNoiseThresh2
shotNoiseThresh2 SetInput [shotNoiseSource GetOutput]
shotNoiseThresh2 ThresholdByLower $shotNoiseFraction
shotNoiseThresh2 SetInValue [expr - $shotNoiseAmplitude]
shotNoiseThresh2 SetOutValue 0.0

vtkImageMathematics shotNoise
shotNoise SetInput1 [shotNoiseThresh1 GetOutput]
shotNoise SetInput2 [shotNoiseThresh2 GetOutput]
shotNoise SetOperationToAdd




vtkImageMathematics add
add SetInput1 [shotNoise GetOutput]
add SetInput2 [imageCanvas GetOutput]
add SetOperationToAdd





vtkImageMedian3D median
median SetInput [add GetOutput]
median SetKernelSize 3 3 1

vtkImageHybridMedian2D hybrid1
hybrid1 SetInput [add GetOutput]

vtkImageHybridMedian2D hybrid2
hybrid2 SetInput [hybrid1 GetOutput]

vtkImageViewer viewer
viewer SetInput [hybrid1 GetOutput]
viewer SetColorWindow 256
viewer SetColorLevel 127.5

viewer Render


