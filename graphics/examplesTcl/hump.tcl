# Create dashed streamlines

source vtkInt.tcl
source colors.tcl
source vtkInclude.tcl

#
# Read data
#
vtkStructuredGridReader reader
   reader SetFileName "../../../data/3d.vtk" ;
   reader Update;#force a read to occur

set dims [[reader GetOutput] GetDimensions]

#
# Outline around data
#
vtkOutlineSource OutlineSource
   OutlineSource SetBounds  -1.2 1.2 -1.2 1.2 0 4
vtkPolyMapper outlineMapper
   outlineMapper SetInput [OutlineSource GetOutput]
vtkActor outline
   outline SetMapper outlineMapper ;
   [outline GetProperty] SetColor .7 .5 .1 ;
#
# Set up shaded surfaces  i.e.  supporting geometry 
#
vtkStructuredGridGeometryFilter humpGeom
   humpGeom SetInput [reader GetOutput]
   eval humpGeom SetExtent 0 [expr [lindex $dims 0] - 1]\
         0 [expr [lindex $dims 1] - 1] 0 0 ;
vtkPolyNormals AddNormals;
   AddNormals SetInput [humpGeom GetOutput]
   AddNormals SetFeatureAngle 60 ;
vtkPolyMapper humpMapper
   humpMapper SetInput [AddNormals GetOutput]
   humpMapper ScalarVisibilityOff;
vtkActor hump
   hump SetMapper humpMapper ;
   [hump GetProperty] SetColor .7 .5 .1 ;
#
# seeds for streamlines
#
set Point1 {-1.0 -1.0 1.0}
set Point2 {-1.0 1.0 1.0}

vtkLineSource StartingPosition
   StartingPosition SetResolution 12
   eval StartingPosition SetPoint1 $Point1 ;
   eval StartingPosition SetPoint2 $Point2 ;

vtkStreamLine streamers
   streamers SetInput [reader GetOutput] ;
   streamers SetSource [StartingPosition GetOutput] ;
#   streamers SetStartPosition -0.568531394 0 1.35339022
   streamers SetMaximumPropagationTime 100000.0
   streamers SetStepLength 0.1 ;
   streamers SetIntegrationStepLength 0.25 ;
   streamers SetIntegrationDirection $VTK_INTEGRATE_FORWARD
   streamers SpeedScalarsOn;

vtkCleanPolyData CleanStrs;
   CleanStrs SetInput [streamers GetOutput]

vtkLookupTable ColorLookupTable
   ColorLookupTable SetHueRange 0.6667 0.0 ;
   ColorLookupTable Build;

vtkTubeFilter tubeF;
   tubeF SetInput [CleanStrs GetOutput]
   tubeF SetRadius 0.025
   tubeF SetNumberOfSides 6 ;
   tubeF SetVaryRadius $VTK_VARY_RADIUS_OFF ;

vtkPolyMapper tubeMapper
   tubeMapper SetLookupTable ColorLookupTable ;
   tubeMapper SetInput [tubeF GetOutput]

vtkActor Tubes
   Tubes SetMapper tubeMapper ;

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor outline ;
ren1 AddActor hump ;
ren1 AddActor Tubes ;

ren1 SetBackground 1 1 1 ;

renWin SetSize 750 750 ;
renWin Render;
iren Initialize;

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract};

renWin Render;

# prevent the tk window from showing up then start the event loop
wm withdraw .


