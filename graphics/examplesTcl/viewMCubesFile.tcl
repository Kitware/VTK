# Generate marching cubes head model (full resolution)

# get the interactor ui and colors
source vtkInt.tcl
source colors.tcl

# Create the render master
vtkRenderMaster rm;

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow];
set ren1   [$renWin MakeRenderer];
set iren [$renWin MakeRenderWindowInteractor];

# prompt for input filename
puts -nonewline "Input marching cubes filename>> "
gets stdin fileName;
puts -nonewline "Input marching cubes limits filename>> "
gets stdin limitsName;

# read from file
vtkMCubesReader reader;
    reader SetFileName $fileName;
    reader SetLimitsFileName $limitsName;
    reader DebugOn;

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

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract};

# prevent the tk window from showing up then start the event loop
wm withdraw .
$iren Initialize;

