# get the interactor ui
source vtkInt.tcl
source "colors.tcl"

# First create the render master
vtkRenderMaster rm;

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow];
set ren1   [$renWin MakeRenderer];
set iren [$renWin MakeRenderWindowInteractor];

# read data
#
vtkPLOT3DReader pl3d;
    pl3d SetXYZFilename "../../data/bluntfinxyz.bin"
    pl3d SetQFilename "../../data/bluntfinq.bin"
    pl3d SetScalarFunctionNumber 100;
    pl3d SetVectorFunctionNumber 202;
    pl3d Update;

# wall
#
vtkStructuredGridGeometryFilter wall;
    wall SetInput [pl3d GetOutput];
    wall SetExtent 0 100 0 0 0 100;
vtkPolyMapper wallMap;
    wallMap SetInput [wall GetOutput];
    wallMap ScalarsVisibleOff;
vtkActor wallActor;
    wallActor SetMapper wallMap;
    eval [wallActor GetProperty] SetColor 0.8 0.8 0.8;

# fin
# 
vtkStructuredGridGeometryFilter fin;
    fin SetInput [pl3d GetOutput];
    fin SetExtent 0 100 0 100 0 0;
vtkPolyMapper finMap;
    finMap SetInput [fin GetOutput];
    finMap ScalarsVisibleOff;
vtkActor finActor;
    finActor SetMapper finMap;
    eval [finActor GetProperty] SetColor 0.8 0.8 0.8;

# texture map
vtkBooleanTexture btext;
    btext SetXSize 20;
    btext SetXSize 20;
vtkTexture texture;
    texture SetInput [btext GetOutput];

# planes to threshold
vtkStructuredGridGeometryFilter plane1;
    plane1 SetInput [pl3d GetOutput];
    plane1 SetExtent 10 10 0 100 0 100;
vtkPolyMapper plane1Map;
    plane1Map SetInput [plane1 GetOutput];
    set pl3dPtData [[pl3d GetOutput] GetPointData];
    set pl3dScalars [$pl3dPtData GetScalars];
    eval plane1Map SetScalarRange [$pl3dScalars GetRange];
vtkActor plane1Actor;
    plane1Actor SetMapper plane1Map;

vtkStructuredGridGeometryFilter plane2;
    plane2 SetInput [pl3d GetOutput];
    plane2 SetExtent 25 25 0 100 0 100;
vtkPolyMapper plane2Map;
    plane2Map SetInput [plane2 GetOutput];
    eval plane2Map SetScalarRange \
      [[[[pl3d GetOutput] GetPointData] GetScalars] GetRange];
vtkActor plane2Actor;
    plane2Actor SetMapper plane2Map;

vtkStructuredGridGeometryFilter plane3;
    plane3 SetInput [pl3d GetOutput];
    plane3 SetExtent 35 35 0 100 0 100;
vtkThresholdTextureCoords thresh3;
    thresh3 SetInput [plane3 GetOutput];
vtkDataSetMapper plane3Map;
    plane3Map SetInput [plane3 GetOutput];
    eval plane3Map SetScalarRange \
      [[[[pl3d GetOutput] GetPointData] GetScalars] GetRange];
vtkActor plane3Actor;
    plane3Actor SetMapper plane3Map;
    plane3Actor SetTexture texture;

# outline
vtkStructuredGridOutlineFilter outline;
    outline SetInput [pl3d GetOutput];
vtkPolyMapper outlineMapper;
    outlineMapper SetInput [outline GetOutput];
vtkActor outlineActor;
    outlineActor SetMapper outlineMapper;
    set outlineProp [outlineActor GetProperty];
    eval $outlineProp SetColor 0 0 0;

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors outlineActor;
$ren1 AddActors wallActor;
$ren1 AddActors finActor;
$ren1 AddActors plane1Actor;
$ren1 AddActors plane2Actor;
$ren1 AddActors plane3Actor;
$ren1 SetBackground 1 1 1;
$renWin SetSize 500 500;

set cam1 [$ren1 GetActiveCamera];
$cam1 Azimuth -40;
$cam1 Zoom 1.4;

$iren Initialize;
$renWin Render;

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract};

# prevent the tk window from showing up then start the event loop
wm withdraw .

#$renWin SetFilename bluntF.tcl.ppm;
#$renWin SaveImageAsPPM;
