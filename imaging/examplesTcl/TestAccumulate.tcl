catch {load vtktcl}

source ../../imaging/examplesTcl/vtkImageInclude.tcl



# Image pipeline

vtkImageReader reader
  reader ReleaseDataFlagOff
  reader SetDataByteOrderToLittleEndian
  reader SetDataExtent 0 255 0 255 1 93
  reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
  reader SetDataMask 0x7fff

vtkImageGaussianSmooth smooth
  smooth SetDimensionality 2
  smooth SetStandardDeviations 1 1
  smooth SetInput [reader GetOutput]

vtkImageAppendComponents append
  append SetInput1 [reader GetOutput]
  append SetInput2 [smooth GetOutput]

vtkImageClip clip
  clip SetInput [append GetOutput]
  clip SetOutputWholeExtent 0 255 0 255 20 22

vtkImageAccumulate accum
  accum SetInput [clip GetOutput]
  accum SetComponentExtent 0 512 0 512 0 0
  accum SetComponentSpacing 6 6 0.0


vtkImageViewer viewer
	viewer SetInput [accum GetOutput]
#	viewer SetZSlice 22
	viewer SetColorWindow 4
	viewer SetColorLevel 2


source ../../imaging/examplesTcl/WindowLevelInterface.tcl









