# Simple viewer for images.
set sliceNumber 5

set VTK_FLOAT              1
set VTK_INT                2
set VTK_SHORT              3
set VTK_UNSIGNED_SHORT     4
set VTK_UNSIGNED_CHAR      5

set VTK_IMAGE_X_AXIS             0
set VTK_IMAGE_Y_AXIS             1
set VTK_IMAGE_Z_AXIS             2
set VTK_IMAGE_TIME_AXIS          3
set VTK_IMAGE_COMPONENT_AXIS     4

# Image pipeline

vtkImageShortReader reader;
#reader DebugOn
	reader ReleaseDataFlagOff;
	reader SwapBytesOff;
	#reader SetDimensions 512 512 60 1;
	reader  SetDimensions 512 512 19 1;
	#reader SetFilePrefix "/home/alyassin2/database/gems/CTangio/CW1/original/i7903CTGE";
	reader SetFilePrefix "/home/alyassin2/database/Duke/cw1/original/duke1";
	reader SetPixelMask 0x7fff;

vtkImageSubSampling ss;
	ss  MaximumOn;
	#ss  SetSamplingFactors 4 4 1;
	ss  SetSamplingFactors 2 2 1;
	ss  SetInput [reader GetOutput];
	ss  ReleaseDataFlagOff;

vtkImageConnectivity connect;
	connect PercentLevelValueOn;
	connect SetPLevelSeedValue 0.9;
	connect SetNeighbors 26;
	connect SetThreshold 1390;
	#connect SingleSeedOn;
	#connect SetSeedXYZ 17 61 5;
	connect SetOutputScalarType $VTK_UNSIGNED_CHAR;
	connect SetInput [ss GetOutput];
	connect ReleaseDataFlagOff;

vtkImageMarkBoundary mb;
	mb SetDilateValue 1;
	mb SetSurfaceValue 100;
	mb SetKernelRadius 1;
	mb SetOutputScalarType $VTK_UNSIGNED_CHAR;
	mb SetInput [connect GetOutput];
	mb ReleaseDataFlagOff;

vtkImageThreshold thresh;
	thresh SetInput [ss GetOutput];
	thresh SetOutputScalarType $VTK_UNSIGNED_SHORT;
	thresh ThresholdByUpper 1390;
	thresh SetInValue 1050;
	thresh ReleaseDataFlagOff;

vtkImageThreshold thresh2;
	thresh2 SetInput [thresh GetOutput];
	thresh2 SetOutputScalarType $VTK_UNSIGNED_SHORT;
	thresh2 ThresholdByLower 1050.0;
	thresh2 SetInValue 1050;
	thresh2 ReleaseDataFlagOff;

vtkImageAdaptiveFilter af;
        af MedianOn;
	af SetAdaptiveValue 100;
	af SetKernelDimensions 1 1 1;
	af SetInput1 [thresh2 GetOutput];
	af SetInput2 [mb GetOutput];
	af ReleaseDataFlagOff;

vtkImageRegion region;
	#region SetExtent 0 127 0 127 0 59;
	region SetExtent 0 255 0 255 0 18;
	[af GetOutput] UpdateRegion region;

vtkImageMIPFilter mip;
	mip SetMinMaxIP 1;
	#mip SetProjectionRange 2 59;
	mip SetProjectionRange 2 18;
	mip SetInput [region GetOutput];
	mip ReleaseDataFlagOff;

vtkImageMagnify mag;
	mag SetDimensionality 2;
	mag SetMagnificationFactors 2 2;
	mag InterpolateOn;
	mag SetInput [mip GetOutput];
	mag ReleaseDataFlagOff;

vtkImageXViewer viewer;
#viewer DebugOn;
	viewer SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS;
	viewer SetInput [mag GetOutput];
	viewer SetCoordinate2 $sliceNumber;
	viewer SetColorWindow 255
	viewer SetColorLevel 127
	#viewer Render;


#make interface
source ui_ctasegmentation.tcl


