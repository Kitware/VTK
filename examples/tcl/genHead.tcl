# Generate marching cubes head model (full resolution)

# get the interactor ui and colors
source vtkInt.tcl
source colors.tcl

# First create the render master
vtkRenderMaster rm;

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow];
set ren1   [$renWin MakeRenderer];
set iren [$renWin MakeRenderWindowInteractor];

# create pipeline
# reader reads slices
vtkVolume16Reader v16;
    v16 SetDataDimensions 256 256
    v16 SwapBytesOn;
    v16 SetFilePrefix "../../data/fullHead/headsq"
    v16 SetDataAspectRatio 0.8 0.8 1.5;
    v16 SetImageRange 1 94;
    v16 SetDataMask 0x7fff;

# write isosurface to file
vtkSliceCubes mcubes;
    mcubes SetReader v16;
    mcubes SetValue 1150;
    mcubes SetFilename "fullHead.tri"
    mcubes SetLimitsFilename "fullHead.lim"
    mcubes Update;

# read from file
vtkMCubesReader reader;
reader SetFilename "fullHead.tri"
reader SetLimitsFilename "fullHead.lim"

vtkPolyMapper mapper;
    mapper SetInput [reader GetOutput];
    
vtkActor head;
    head SetMapper mapper;
    eval [head GetProperty] SetColor $raw_sienna;

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors head;
$ren1 SetBackground 1 1 1;
$renWin SetSize 500 500;
eval $ren1 SetBackground $slate_grey;
[$ren1 GetActiveCamera] Zoom 1.5;
[$ren1 GetActiveCamera] Azimuth 180;
[$ren1 GetActiveCamera] Elevation -90;

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract};

$iren Initialize;

#$renWin SetFilename "genHead.tcl.ppm";
#$renWin SaveImageAsPPM;

# prevent the tk window from showing up then start the event loop
wm withdraw .

