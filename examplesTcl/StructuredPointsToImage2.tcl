# Convert a structured points data set into an image.
# Display the image in an XViewer.


source ../imaging/examplesTcl/vtkImageInclude.tcl


vtkVolume16Reader v16
    v16 SetDataDimensions 256 256
    v16 SetFilePrefix "../data/fullHead/headsq";
    v16 SetDataOrigin -127.5 -127.5 5.33
    v16 SetImageRange 1 90
    v16 SetDataSpacing 1 1 5.33
    v16 SwapBytesOn
    v16 Update

vtkImageXViewer viewer;
#viewer DebugOn;
viewer SetInput [v16 GetOutput];
viewer SetZSlice 22;
viewer SetColorWindow 3000
viewer SetColorLevel 1500
viewer Render;


# make interface
source ../imaging/examplesTcl/WindowLevelInterface.tcl




