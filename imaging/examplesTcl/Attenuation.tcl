# Show the constant kernel.  Smooth an impulse function.

catch {load vtktcl}

source vtkImageInclude.tcl

vtkPNMReader reader
reader ReleaseDataFlagOff
reader SetFileName "../../../vtkdata/AttenuationArtifact.pgm"

vtkImageThreshold cast
cast ThresholdByLower 0
cast SetInput [reader GetOutput]
cast SetOutputScalarTypeToFloat

# get rid of discrete scalars
vtkImageGaussianSmooth smooth
smooth SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
smooth SetInput [cast GetOutput]
smooth SetStandardDeviations 0.8 0.8

vtkSphere m1
m1 SetCenter 310 130 0
m1 SetRadius 0

vtkSampleFunction m2
m2 SetImplicitFunction m1
m2 SetModelBounds 0 264 0 264 0 1
m2 SetSampleDimensions 264 264 1

vtkImageShiftScale m3
m3 SetInput [m2 GetOutput]
m3 SetScale 0.000095

vtkImageMathematics m4
m4 SetInput1 [m3 GetOutput]
m4 SetOperationToSquare
m4 BypassOn



vtkImageMathematics m5
m5 SetInput1 [m4 GetOutput]
m5 SetOperationToInvert

vtkImageShiftScale m6
m6 SetInput [m5 GetOutput]
m6 SetScale 255

vtkImageShiftScale t2
#t2 SetInput [t1 GetOutput]
t2 SetScale -1

#vtkImageMathematics m3
#m3 SetInput1 [t2 GetOutput]
#m3 SetOperationToInvert
#m3 ReleaseDataFlagOff

vtkImageMathematics div
div SetInput1 [smooth GetOutput]
div SetInput2 [m3 GetOutput]
div SetOperationToMultiply

vtkImageViewer viewer
viewer SetInput [div GetOutput]
#viewer SetInput [cast GetOutput]
viewer SetColorWindow 256
viewer SetColorLevel 128
#viewer ColorFlagOn

# make interface
source WindowLevelInterface.tcl




