# Converts gradient vectors and magnitude into a structured points with
# scalars and vectors.  It displays them with hedge hogs.

# Now create the RenderWindow and Renderer.
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

## Create an image pipeline
vtkImageSeriesReader reader;
reader SwapBytesOn;
reader SetDataDimensions 256 256 93;
reader SetFilePrefix "../data/fullHead/headsq"
reader SetPixelMask 0x7fff;
reader SetOutputScalarType 1;
#reader DebugOn

vtkImageGradient gradient;
gradient SetInput [reader GetOutput];
gradient SetAxes 0 1 2;
gradient ReleaseDataFlagOff;

vtkImageMagnitude magnitude;
magnitude SetInput [gradient GetOutput];

vtkImageToStructuredPoints image;
image SetScalarInput [magnitude GetOutput];
image SetVectorInput [gradient GetOutput];
image SetExtent 0 255 0 255 20 20;
image SetAxes 0 1 2; 






# create vis pipeline
#
vtkHedgeHog hhog;
    hhog SetInput [image GetOutput];
    hhog SetScaleFactor 0.002;
vtkLookupTable lut;
#    lut SetHueRange 0 10;
    lut Build;
vtkPolyMapper hhogMapper;
    hhogMapper SetInput [hhog GetOutput];
    hhogMapper SetScalarRange 50 2000;
    hhogMapper SetLookupTable lut;
vtkActor hhogActor;
    hhogActor SetMapper hhogMapper;

vtkOutlineFilter outline;
    outline SetInput [image GetOutput];
vtkPolyMapper outlineMapper;
    outlineMapper SetInput [outline GetOutput];
vtkActor outlineActor;
    outlineActor SetMapper outlineMapper;
set outlineProp [outlineActor GetProperty];
#eval $outlineProp SetColor 0 0 0;

# Add the actors to the renderer, set the background and size
#
ren1 AddActors outlineActor;
ren1 AddActors hhogActor;
ren1 SetBackground 1 1 1;
renWin SetSize 500 500;
#renWin SetSize 1000 1000;
ren1 SetBackground 0.1 0.2 0.4;
iren Initialize;

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract};
[ren1 GetActiveCamera] Zoom 1.5;
renWin Render;
#renWin SetFileName "complexV.tcl.ppm";
#renWin SaveImageAsPPM;

# prevent the tk window from showing up then start the event loop
wm withdraw .


