# Show the constant kernel.  Smooth an impulse function.

#set shotNoiseAmplitude 2000.0
#set shotNoiseFraction 0.1
#set shotNoiseExtent 0 255 0 255 0 92

catch {load vtktcl}

source vtkImageInclude.tcl

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


