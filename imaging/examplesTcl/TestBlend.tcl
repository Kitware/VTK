catch {load vtktcl}

source vtkImageInclude.tcl

# do alpha-blending of two images

vtkPNMReader reader1
reader1 SetFileName "../../../vtkdata/masonry.ppm"

vtkPNMReader reader2
reader2 SetFileName "../../../vtkdata/B.pgm"

vtkLookupTable table
table SetTableRange 0 127 
table SetValueRange 0.0 1.0 
table SetSaturationRange 0.0 0.0 
table SetHueRange 0.0 0.0 
table SetAlphaRange 0.9 0.0 
table Build

vtkImageMapToColors rgba
rgba SetInput [reader2 GetOutput]
rgba SetLookupTable table 

vtkImageTranslateExtent translate
translate SetInput [rgba GetOutput]
translate SetTranslation 60 60 0

vtkImageBlend blend
blend SetInput 0 [reader1 GetOutput]
blend SetInput 1 [translate GetOutput]
#blend SetOpacity 1 0.5 

# set the window/level to 255.0/127.5 to view full range
vtkImageViewer viewer
viewer SetInput [blend GetOutput]
viewer SetColorWindow 255.0
viewer SetColorLevel 127.5
viewer SetZSlice 50

viewer Render

#make interface
source WindowLevelInterface.tcl

vtkWindowToImageFilter windowToimage
  windowToimage SetInput [viewer GetImageWindow]

vtkPNMWriter pnmWriter
  pnmWriter SetInput [windowToimage GetOutput]
  pnmWriter SetFileName "TestBlend.tcl.ppm"
#  pnmWriter Write

