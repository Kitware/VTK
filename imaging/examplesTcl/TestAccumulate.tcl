catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


source ../../imaging/examplesTcl/vtkImageInclude.tcl



# Image pipeline

vtkImageReader reader
  reader ReleaseDataFlagOff
  reader SetDataByteOrderToLittleEndian
  reader SetDataExtent 0 255 0 255 1 93
  reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
  reader SetDataMask 0x7fff

vtkImageGaussianSmooth smooth
  smooth SetDimensionality 2
  smooth SetStandardDeviations 1 1
  smooth SetInput [reader GetOutput]

vtkImageAppendComponents imageAppend
  imageAppend SetInput 0 [reader GetOutput]
  imageAppend SetInput 1 [smooth GetOutput]

vtkImageClip clip
  clip SetInput [imageAppend GetOutput]
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









