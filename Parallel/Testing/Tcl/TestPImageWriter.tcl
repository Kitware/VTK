package require vtk

# Image pipeline

vtkTIFFReader image1
  image1 SetFileName "$VTK_DATA_ROOT/Data/beach.tif"
  image1 Update

#
# If the current directory is writable, then test the witers
#

if {[catch {set channel [open test.tmp w]}] == 0 } {
    close $channel
    file delete -force test.tmp
    
    vtkPImageWriter piw
    piw SetInput [image1 GetOutput]
    piw SetFileName piw.raw
    piw SetMemoryLimit 1
    
    piw Write

    file delete -force piw.raw
}

vtkImageViewer viewer
viewer SetInput [image1 GetOutput]
viewer SetColorWindow 255
viewer SetColorLevel 127.5

wm withdraw .

viewer Render


