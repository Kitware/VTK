catch {load vtktcl}
# Diffuses to 26 neighbors if difference is below threshold.

source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
reader SetDataSpacing 1 1 2 
#reader DebugOn


vtkImageAnisotropicDiffusion3D diffusion
diffusion SetInput [reader GetOutput]
diffusion SetDiffusionFactor 1.0
diffusion SetDiffusionThreshold 100.0
diffusion SetNumberOfIterations 5
diffusion ReleaseDataFlagOff


vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [diffusion GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 3000
viewer SetColorLevel 1500


#make interface
source WindowLevelInterface.tcl







