catch {load vtktcl}
# Make an image larger by repeating the data.  Tile.


source vtkImageInclude.tcl


# Image pipeline
vtkPNMReader reader
reader ReleaseDataFlagOff
reader SetFileName "../../../vtkdata/earth.ppm"

vtkImageWrapPad pad
pad SetInput [reader GetOutput]
pad SetOutputWholeExtent -100 100 0 250 0 0

vtkImageViewer viewer
viewer SetInput [pad GetOutput]
viewer SetZSlice 0
viewer SetColorWindow 255
viewer SetColorLevel 127
[viewer GetActor2D] SetDisplayPosition 100 0

#make interface
source WindowLevelInterface.tcl







