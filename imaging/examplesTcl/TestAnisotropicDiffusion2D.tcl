catch {load vtktcl}

source vtkImageInclude.tcl

# Image pipeline

vtkImageVolume16Reader reader
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
reader SetOutputScalarType $VTK_FLOAT
#reader DebugOn

vtkImageAnisotropicDiffusion2D diffusion
diffusion SetInput [reader GetOutput]
diffusion SetDiffusionFactor 1.0
diffusion SetDiffusionThreshold 200.0
diffusion SetNumberOfIterations 5
diffusion ReleaseDataFlagOff
#diffusion DebugOn


vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [diffusion GetOutput]
viewer SetCoordinate2 22
viewer SetColorWindow 3000
viewer SetColorLevel 1500


source WindowLevelInterface.tcl




