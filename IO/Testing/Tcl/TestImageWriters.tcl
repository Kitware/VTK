package require vtk

# Image pipeline

vtkTIFFReader image1
  image1 SetFileName "$VTK_DATA_ROOT/Data/beach.tif"
  image1 Update

vtkStructuredPoints sp
eval sp SetDimensions [[image1 GetOutput] GetDimensions]
eval sp SetExtent [[image1 GetOutput] GetExtent]
sp SetScalarType [[image1 GetOutput] GetScalarType] 
sp SetNumberOfScalarComponents [[image1 GetOutput] GetNumberOfScalarComponents] 
[sp GetPointData] SetScalars [[[image1 GetOutput] GetPointData] GetScalars]

vtkImageLuminance luminance
  luminance SetInput sp

#
# If the current directory is writable, then test the witers
#

if {[catch {set channel [open test.tmp w]}] == 0 } {
   close $channel
   file delete -force test.tmp

   vtkTIFFWriter tiff1
   tiff1 SetInput [image1 GetOutput]
   tiff1 SetFileName tiff1.tif
   
   vtkTIFFWriter tiff2
   tiff2 SetInput [luminance GetOutput]
   tiff2 SetFileName tiff2.tif
   
   vtkBMPWriter bmp1
   bmp1 SetInput [image1 GetOutput]
   bmp1 SetFileName bmp1.bmp
   
   vtkBMPWriter bmp2
   bmp2 SetInput [luminance GetOutput]
   bmp2 SetFileName bmp2.bmp
   
   vtkPNMWriter pnm1
   pnm1 SetInput [image1 GetOutput]
   pnm1 SetFileName pnm1.pnm
   
   vtkPNMWriter pnm2
   pnm2 SetInput [luminance GetOutput]
   pnm2 SetFileName pnm2.pnm

   vtkPostScriptWriter psw1
   psw1 SetInput [image1 GetOutput]
   psw1 SetFileName psw1.ps

   vtkPostScriptWriter psw2
   psw2 SetInput [luminance GetOutput]
   psw2 SetFileName psw2.ps

   tiff1 Write
   tiff2 Write
   bmp1 Write
   bmp2 Write
   pnm1 Write
   pnm2 Write
   psw1 Write
   psw2 Write

   file delete -force tiff1.tif
   file delete -force tiff2.tif
   file delete -force bmp1.bmp
   file delete -force bmp2.bmp
   file delete -force pnm1.pnm
   file delete -force pnm2.pnm
   file delete -force psw1.ps
   file delete -force psw2.ps
}

vtkImageViewer viewer
  viewer SetInput [luminance GetOutput]
  viewer SetColorWindow 255
  viewer SetColorLevel 127.5
  viewer Render

