catch {load vtktcl}

source ../../imaging/examplesTcl/vtkImageInclude.tcl



# Image pipeline

vtkImageReader reader
  reader ReleaseDataFlagOff
  reader SetDataByteOrderToLittleEndian
  reader SetDataExtent 0 255 0 255 1 93
  reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
  reader SetDataMask 0x7fff
  reader DebugOn


vtkImageGaussianSmooth smooth
  smooth SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
  smooth SetStandardDeviations 1 1
  smooth SetInput [reader GetOutput]

vtkImageAppendComponents append
  append SetInput1 [reader GetOutput]
  append SetInput2 [smooth GetOutput]

vtkImageClip clip
  clip SetInput [append GetOutput]
  clip SetOutputWholeExtent 0 255 0 255 20 22 0 0

vtkImageAccumulate splat
  splat SetOutputScalarTypeToInt
  splat SetInput [clip GetOutput]
  splat SetComponentExtent 0 512 0 512
  splat SetComponentSpacing 6 6 


vtkImageViewer viewer
	#viewer DebugOn
	viewer SetInput [splat GetOutput]
	viewer SetCoordinate2 22
	viewer SetColorWindow 4
	viewer SetColorLevel 2


source ../../imaging/examplesTcl/WindowLevelInterface.tcl









