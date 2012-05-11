#import "MyDocument.h"

#import "BasicVTKView.h"

#import "vtkInteractorStyleSwitch.h"
#import "vtkCocoaRenderWindowInteractor.h"
#import "vtkConeSource.h"
#import "vtkCylinderSource.h"
#import "vtkPolyDataMapper.h"
#import "vtkSmartPointer.h"
#import "vtkDebugLeaks.h"

@implementation MyDocument

- (void)setupLeftVTKView
{
    [leftVTKView initializeVTKSupport];

    // 'smart pointers' are used because they are very similiar to creating auto-realeased objects in Cocoa.

    // Personal Taste Section. I like to use a trackball interactor
    vtkSmartPointer<vtkInteractorStyleSwitch>   intStyle = vtkSmartPointer<vtkInteractorStyleSwitch>::New();
    intStyle->SetCurrentStyleToTrackballCamera();
    [leftVTKView getInteractor]->SetInteractorStyle(intStyle);

    // Create a cone, see the "VTK User's Guide" for details
    vtkSmartPointer<vtkConeSource> cone = vtkSmartPointer<vtkConeSource>::New();
        cone->SetHeight(3.0);
        cone->SetRadius(1.0);
        cone->SetResolution(100);
    vtkSmartPointer<vtkPolyDataMapper>  coneMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        coneMapper->SetInputConnection(cone->GetOutputPort());
    vtkSmartPointer<vtkActor>   coneActor = vtkSmartPointer<vtkActor>::New();
        coneActor->SetMapper(coneMapper);
    [leftVTKView getRenderer]->AddActor(coneActor);

    // Tell the system that the view needs to be redrawn
    [leftVTKView setNeedsDisplay:YES];
}

- (void)setupRightVTKView
{
    [rightVTKView initializeVTKSupport];

    // 'smart pointers' are used because they are very similiar to creating auto-realeased objects in Cocoa.

    // Personal Taste Section. I like to use a trackball interactor
    vtkSmartPointer<vtkInteractorStyleSwitch>   intStyle = vtkSmartPointer<vtkInteractorStyleSwitch>::New();
    intStyle->SetCurrentStyleToTrackballCamera();
    [rightVTKView getInteractor]->SetInteractorStyle(intStyle);

    // Create a cyclinder, see the "VTK User's Guide" for details
    vtkSmartPointer<vtkCylinderSource>  cylinder = vtkSmartPointer<vtkCylinderSource>::New();
        cylinder->SetResolution(100);
    vtkSmartPointer<vtkPolyDataMapper>  cylinderMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        cylinderMapper->SetInputConnection(cylinder->GetOutputPort());
    vtkSmartPointer<vtkActor>   cylinderActor = vtkSmartPointer<vtkActor>::New();
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

- (void)close
{
    [super close];

    // Releases memory allocated in initializeVTKSupport.
    // In a way, calling these is pointless since the application is quitting anyway.
    [leftVTKView cleanUpVTKSupport];
    [rightVTKView cleanUpVTKSupport];

    // If you have built vtk with VTK_DEBUG_LEAKS on then this method will print out any leaks
    // that exist.  The sample has been careful to cleanup after itself, so there should be no leaks.
    vtkDebugLeaks::PrintCurrentLeaks();
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
