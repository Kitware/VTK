package require vtk





# Image pipeline

vtkPNGReader reader
reader SetFileName $VTK_DATA_ROOT/Data/fullhead15.png

vtkImageGaussianSmooth smooth
  smooth SetDimensionality 2
  smooth SetStandardDeviations 1 1
  smooth SetInput [reader GetOutput]

vtkImageAppendComponents imageAppend
  imageAppend AddInput [reader GetOutput]
  imageAppend AddInput [smooth GetOutput]

vtkImageClip clip
  clip SetInput [imageAppend GetOutput]
  clip SetOutputWholeExtent 0 255 0 255 20 22

vtkImageAccumulate accum
  accum SetInput [clip GetOutput]
  accum SetComponentExtent 0 255 0 255 0 0
  accum SetComponentSpacing 12 12 0.0


vtkImageViewer viewer
	viewer SetInput [accum GetOutput]
	viewer SetColorWindow 4
	viewer SetColorLevel 2


viewer Render









