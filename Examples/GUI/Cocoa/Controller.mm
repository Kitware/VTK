#import "Controller.h"


#define id Id
#include "vtkProperty.h"
#include "vtkActor.h"
#include "vtkOutlineFilter.h"
#include "vtkTubeFilter.h"
#include "vtkSphereSource.h"
#include "vtkDataSetMapper.h"
#include "vtkElevationFilter.h"
#include "vtkCamera.h"
#undef id

//Most of this code was taken from "The Visualization Toolkit, 3rd Edition, page 99/100
@implementation Controller

- (IBAction)makeData:(id)sender
{
  // Create a sphere source
  vtkSphereSource *sphere = vtkSphereSource::New();
  sphere->SetPhiResolution(12);
  sphere->SetThetaResolution(12);
  
  // Create something to color the data
  vtkElevationFilter *colorIt = vtkElevationFilter::New();
  colorIt->SetInput(sphere->GetOutput() );
  colorIt->SetLowPoint(0,0,-1);
  colorIt->SetHighPoint(0,0,1);
  
  //Create something to map the data
  vtkDataSetMapper *mapper = vtkDataSetMapper::New();
  mapper->SetInput(colorIt->GetOutput() );
  
  // Create a 3D actor for the Data
  vtkActor *actor = vtkActor::New();
  actor->SetMapper(mapper);
  // Make it look nicer
  actor->GetProperty()->SetInterpolation( VTK_GOURAUD );
  actor->GetProperty()->SetRepresentationToSurface(); 
  
  /* This sets the center of rotation to the center of the sphere. This will not
    have much effect on a simple sphere, but other imported geometries may not
    be positioned so nicely. I struggled with this code, so I offer here for
    other newbies.
    */
  double center[3];
  sphere->GetCenter(center);
  center[0] *= -1.0; center[1] *= -1.0; center[2]  *= -1.0;
  actor->SetPosition(center);
  
  //Add the Actor to the rederer
  [vtkView renderer]->AddActor(actor);
  
  // Set the position and focal point of the Camera so that we are "outside"
  //   the sphere
  [vtkView renderer]->GetActiveCamera()->SetPosition(center[0],center[1],-2.25);
  [vtkView renderer]->GetActiveCamera()->SetFocalPoint(center);
  [vtkView renderer]->ResetCamera();
  
  //Tell the NSView to Display NOW. We could do this lazily and 
  //  use [vtkView setNeedsDisplay:YES]; instead, but in this simple example 
  //  you would have to manually move or resize the window to get a refresh of
  //  the NSView. If you already have a render or animation thread going it might
  //  be better to use the "setNeedsDisplay:YES" instead of "display"
  [vtkView display];
   
  //Clean up memory
  sphere->Delete();
  mapper->Delete();
  actor->Delete();
  // Disable the button so we can not click it again. 
  [go setEnabled:NO];
}

@end
