package require vtk



# Image pipeline

vtkPNGReader reader
reader SetFileName $VTK_DATA_ROOT/Data/fullhead15.png

vtkImageAnisotropicDiffusion2D diffusion
diffusion SetInput [reader GetOutput]
diffusion SetDiffusionFactor 1.0
diffusion SetDiffusionThreshold 200.0
diffusion SetNumberOfIterations 5
#diffusion DebugOn

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [diffusion GetOutput]
viewer SetColorWindow 3000
viewer SetColorLevel 1500


viewer Render




