# This Script test the euclidean to polar by coverting 2D vectors 
# from a gradient into polar, which is converted into HSV, and then to RGB.
catch {load vtktcl}

source vtkImageInclude.tcl


# Image pipeline

vtkImageReader reader
#reader DebugOn
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageEllipsoidSource ellipsoid
ellipsoid SetWholeExtent 0 255 0 255 0 44
ellipsoid SetCenter 127 127 22
ellipsoid SetRadius 100 100 100
ellipsoid SetOutValue 0
ellipsoid SetInValue 200
ellipsoid SetOutputScalarType $VTK_FLOAT

vtkImageGaussianSource gauss
gauss SetWholeExtent 0 255 0 255 0 44
gauss SetCenter 127 127 22
gauss SetStandardDeviation 50.0
gauss SetMaximum 8000.0

vtkImageGradient gradient
gradient SetInput [reader GetOutput]
#gradient SetInput [ellipsoid GetOutput]
#gradient SetInput [gauss GetOutput]
gradient SetDimensionality 2
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

#make interface
source WindowLevelInterface.tcl







