# This Script test the euclidean to polar by coverting 2D vectors 
# from a gradient into polar, which is converted into HSV, and then to RGB.
catch {load vtktcl}

source vtkImageInclude.tcl


# Image pipeline

vtkImageVolume16Reader reader
#reader DebugOn
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageElipsoidSource elipsoid
elipsoid SetWholeExtent 0 255 0 255 0 44
elipsoid SetCenter 127 127 22
elipsoid SetRadius 100 100 100
elipsoid SetOutValue 0
elipsoid SetInValue 200
elipsoid SetOutputScalarTypeToFloat

vtkImageGaussianSource gauss
gauss SetWholeExtent 0 255 0 255 0 44 0 0
gauss SetCenter 127 127 22 0
gauss SetStandardDeviation 50.0
gauss SetMaximum 8000.0

vtkImageGradient gradient
gradient SetInput [reader GetOutput]
#gradient SetInput [elipsoid GetOutput]
#gradient SetInput [gauss GetOutput]
gradient SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
gradient ReleaseDataFlagOff

vtkImageEuclideanToPolar polar
polar SetInput [gradient GetOutput]
polar SetThetaMaximum 255

vtkImageConstantPad pad
pad SetInput [polar GetOutput]
pad SetOutputNumberOfScalarComponents 3
pad SetConstant 200

# permute components so saturation will be constant
vtkImageExtractComponents permute
permute SetInput [pad GetOutput]
permute SetComponents 0 2 1

vtkImageHSVToRGB rgb
rgb SetInput [permute GetOutput]
rgb SetMaximum 255

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [rgb GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 255
viewer SetColorLevel 127
#viewer SetRedComponent 1
#viewer SetBlueComponent 1
#viewer SetGreenComponent 1
viewer ColorFlagOn


#make interface
source WindowLevelInterface.tcl







