package require vtk

# Diffuses to 26 neighbors if difference is below threshold.


# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 63 0 63 1 93
reader SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
reader SetDataMask 0x7fff
reader SetDataSpacing 1 1 2 


vtkImageAnisotropicDiffusion3D diffusion
diffusion SetInputConnection [reader GetOutputPort]
diffusion SetDiffusionFactor 1.0
diffusion SetDiffusionThreshold 100.0
diffusion SetNumberOfIterations 5
diffusion ReleaseDataFlagOff


vtkImageViewer viewer
#viewer DebugOn
viewer SetInputConnection [diffusion GetOutputPort]
viewer SetZSlice 22
viewer SetColorWindow 3000
viewer SetColorLevel 1500


#make interface
viewer Render







