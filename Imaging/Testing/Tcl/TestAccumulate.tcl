package require vtk





# Image pipeline

vtkPNGReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/fullhead15.png"

vtkImageGaussianSmooth smooth
  smooth SetDimensionality 2
  smooth SetStandardDeviations 1 1
  smooth SetInputConnection [reader GetOutputPort]

vtkImageAppendComponents imageAppend
  imageAppend AddInputConnection [reader GetOutputPort]
  imageAppend AddInputConnection [smooth GetOutputPort]

vtkImageClip clip
  clip SetInputConnection [imageAppend GetOutputPort]
  clip SetOutputWholeExtent 0 255 0 255 20 22

vtkImageAccumulate accum
  accum SetInputConnection [clip GetOutputPort]
  accum SetComponentExtent 0 255 0 255 0 0
  accum SetComponentSpacing 12 12 0.0


vtkImageViewer viewer
	viewer SetInputConnection [accum GetOutputPort]
	viewer SetColorWindow 4
	viewer SetColorLevel 2


viewer Render









