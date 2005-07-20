package require vtk



# Image pipeline

vtkPNGReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/fullhead15.png"

vtkImageAnisotropicDiffusion2D diffusion
diffusion SetInputConnection [reader GetOutputPort]
diffusion SetDiffusionFactor 1.0
diffusion SetDiffusionThreshold 200.0
diffusion SetNumberOfIterations 5
#diffusion DebugOn

vtkImageViewer viewer
#viewer DebugOn
viewer SetInputConnection [diffusion GetOutputPort]
viewer SetColorWindow 3000
viewer SetColorLevel 1500


viewer Render



