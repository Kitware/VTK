# Developed By Majeid Alyassin

set slicenumber       0
set firstslice        5
set numslices         5
set thresholdvalue 1400
set invalue        1000
set boundaryvalue   100
set kernelradius      1
set subx 4;
set suby 4;
set subz 1;
set subs 1;
set afilter 10;
set con 6;
set lprojection $subz;
set uprojection [expr $numslices - 1 - $subz];
set window 2000;
set level 2000;
#set prefix "/home/alyassin2/database/Duke/cw1/original/duke1";
set prefix "/home/alyassin2/database/gems/CTangio/CW1/original/i7903CTGE";

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
	reader  SetDimensions 512 512 $numslices 1;
        reader  SetFirst $firstslice;
	reader  SetFilePrefix $prefix;
	reader  SetOutputScalarType $VTK_SHORT;
	reader  SetPixelMask 0x7fff;
	reader  ReleaseDataFlagOff;

vtkImageSubSampling ss;
	ss  MaximumOn;
	ss  SetSamplingFactors $subx $suby $subz;
	ss  SetInput [reader GetOutput];
	ss  ReleaseDataFlagOff;

vtkImageConnectivity connect;
	connect PercentLevelValueOn;
	connect SetPLevelSeedValue 0.9;
	connect SetNeighbors 26;
	connect SetThreshold $thresholdvalue;
	#connect SingleSeedOn;
	#connect SetSeedXYZ 17 61 5;
	connect SetOutputScalarType $VTK_UNSIGNED_CHAR;
	connect SetInput [ss GetOutput];
	connect ReleaseDataFlagOff;

vtkImageMarkBoundary mb;
	mb SetDilateValue 1;
	mb SetSurfaceValue $boundaryvalue;
	mb SetKernelRadius $kernelradius;
	mb SetOutputScalarType $VTK_UNSIGNED_CHAR;
	mb SetInput [connect GetOutput];
	mb ReleaseDataFlagOff;

vtkImageThreshold thresh;
	thresh SetInput [ss GetOutput];
	thresh SetOutputScalarType $VTK_SHORT;
	thresh ThresholdByUpper $thresholdvalue;
	thresh SetInValue $invalue;
	thresh ReleaseDataFlagOff;

vtkImageThreshold thresh2;
	thresh2 SetInput [thresh GetOutput];
	thresh2 SetOutputScalarType $VTK_SHORT;
	thresh2 ThresholdByLower $invalue;
	thresh2 SetInValue $invalue;
	thresh2 ReleaseDataFlagOff;

vtkImageAdaptiveFilter af;
        af MedianOn;
	af SetAdaptiveValue $boundaryvalue;
	af SetKernelDimensions $kernelradius $kernelradius $kernelradius;
	af SetInput1 [thresh2 GetOutput];
	af SetInput2 [mb GetOutput];
	af SetOutputScalarType $VTK_SHORT;
	af ReleaseDataFlagOff;

vtkImageCTAComposite composite;
	composite SetMaskInput       [mb     GetOutput];
	composite SetOriginalInput   [reader GetOutput];
	composite SetSubSampledInput [af     GetOutput];
	composite SetMagnificationFactors $subx $suby $subz;
	composite SetReplaceValue $invalue;
	composite SetMaskReplace  1;
	composite SetMaskOriginal 0;
	composite SetMaskTrilinear $boundaryvalue;
	composite ReleaseDataFlagOff;


vtkImageMIPFilter mip;
	mip SetMinMaxIP 1;
	mip SetProjectionRange  $lprojection $uprojection;
	mip SetInput [composite GetOutput];
	mip ReleaseDataFlagOff;

vtkImageXViewer viewer;
	viewer SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS;
	viewer SetInput [mip GetOutput];
	viewer SetCoordinate2 0;
	viewer SetColorWindow $window;
	viewer SetColorLevel $level;
	viewer SetXOffset 512
        #viewer Render;

vtkImageXViewer viewer1;
	viewer1 SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS;
	viewer1 SetInput [ss GetOutput];
	viewer1 SetCoordinate2 0;
	viewer1 SetColorWindow $window;
	viewer1 SetColorLevel $level;
	viewer1 SetXOffset 0;
	viewer1 SetWindow [viewer GetWindow];
	#viewer1 Render;

vtkImageToStructuredPoints image;
        image SetScalarInput [composite GetOutput];

vtkStructuredPointsWriter writer;
        writer SetInput [image GetOutput];
	writer SetFileType 2;
        writer SetFilename "CTAStrucutredPoints.bin";



#make interface
source CTASegmentation_main_ui.tcl
source CTASegmentation_browser_ui.tcl;
source CTASegmentation_wl_ui.tcl
source CTASegmentation_reader_param_ui.tcl;


