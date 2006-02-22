#import "MyDocument.h"

#import "BasicVTKView.h"

#define id Id
#include "vtkInteractorStyleSwitch.h"
#include "vtkCocoaRenderWindowInteractor.h"
#include "vtkConeSource.h"
#include "vtkCylinderSource.h"
#include "vtkPolyDataMapper.h"
#undef id

@implementation MyDocument

- (void)setupLeftVTKView
{
  [leftVTKView initializeVTKSupport];

  // Personal Taste Section. I like to use a trackball interactor
  vtkInteractorStyleSwitch*  intStyle = vtkInteractorStyleSwitch::New();
  intStyle->SetCurrentStyleToTrackballCamera();
  [leftVTKView getInteractor]->SetInteractorStyle(intStyle);
  intStyle->Delete();

  // Create a cone, see the "VTK User's Guide" for details
  vtkConeSource*    cone = vtkConeSource::New();
    cone->SetHeight(3.0);
    cone->SetRadius(1.0);
    cone->SetResolution(100);
  vtkPolyDataMapper*  coneMapper = vtkPolyDataMapper::New();
    coneMapper->SetInput(cone->GetOutput());
  vtkActor*  coneActor = vtkActor::New();
    coneActor->SetMapper(coneMapper);
    [leftVTKView getRenderer]->AddActor(coneActor);
  
  // Tell the system that the view needs to be redrawn
  [leftVTKView setNeedsDisplay:YES];
}

- (void)setupRightVTKView
{
  [rightVTKView initializeVTKSupport];

  // Personal Taste Section. I like to use a trackball interactor
  vtkInteractorStyleSwitch*  intStyle = vtkInteractorStyleSwitch::New();
  intStyle->SetCurrentStyleToTrackballCamera();
  [rightVTKView getInteractor]->SetInteractorStyle(intStyle);
  intStyle->Delete();

  // Create a cyclinder, see the "VTK User's Guide" for details
  vtkCylinderSource*    cylinder = vtkCylinderSource::New();
    cylinder->SetResolution(100);
  vtkPolyDataMapper*  cylinderMapper = vtkPolyDataMapper::New();
    cylinderMapper->SetInput(cylinder->GetOutput());
  vtkActor*  cylinderActor = vtkActor::New();
    cylinderActor->SetMapper(cylinderMapper);
    [rightVTKView getRenderer]->AddActor(cylinderActor);

  // Tell the system that the view needs to be redrawn
  [rightVTKView setNeedsDisplay:YES];
}

#pragma mark -

- (id)init 
{
    self = [super init];
    if (self != nil) {
        // Add your subclass-specific initialization here.
        // If an error occurs here, send a [self release] message and return nil.
    }
    return self;
}

- (void)applicationWillTerminate:(NSNotification*)aNotification
{
  [leftVTKView cleanUpVTKSupport];
  [rightVTKView cleanUpVTKSupport];
}

- (NSString *)windowNibName 
{
    // Override returning the nib file name of the document
    // If you need to use a subclass of NSWindowController or if your document supports multiple NSWindowControllers, you should remove this method and override -makeWindowControllers instead.
    return @"MyDocument";
}

- (void)windowControllerDidLoadNib:(NSWindowController *)windowController 
{
    [super windowControllerDidLoadNib:windowController];
  
    // vtk stuff
  [self setupLeftVTKView];
  [self setupRightVTKView];
}


- (NSData *)dataRepresentationOfType:(NSString *)aType
{
    // Insert code here to write your document from the given data.  You can also choose to override -fileWrapperRepresentationOfType: or -writeToFile:ofType: instead.
    
    // For applications targeted for Tiger or later systems, you should use the new Tiger API -dataOfType:error:.  In this case you can also choose to override -writeToURL:ofType:error:, -fileWrapperOfType:error:, or -writeToURL:ofType:forSaveOperation:originalContentsURL:error: instead.

    return nil;
}

- (BOOL)loadDataRepresentation:(NSData *)data ofType:(NSString *)aType
{
    // Insert code here to read your document from the given data.  You can also choose to override -loadFileWrapperRepresentation:ofType: or -readFromFile:ofType: instead.

    // For applications targeted for Tiger or later systems, you should use the new Tiger API readFromData:ofType:error:.  In this case you can also choose to override -readFromURL:ofType:error: or -readFromFileWrapper:ofType:error: instead.
    
    return YES;
}

@end
