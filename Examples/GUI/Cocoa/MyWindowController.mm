#import "MyWindowController.h"

#import "BasicVTKView.h"

#import "vtkInteractorStyleSwitch.h"
#import "vtkCocoaRenderWindowInteractor.h"
#import "vtkConeSource.h"
#import "vtkCylinderSource.h"
#import "vtkPolyDataMapper.h"
#import "vtkSmartPointer.h"

// Private Interface
@interface MyWindowController()
@property (readwrite, weak, nonatomic) IBOutlet BasicVTKView* leftVTKView;
@property (readwrite, weak, nonatomic) IBOutlet BasicVTKView* rightVTKView;
@end

@implementation MyWindowController

// ----------------------------------------------------------------------------
- (void)setupLeftVTKView
{
  [[self leftVTKView] initializeVTKSupport];

  // 'smart pointers' are used because they are very similar to reference counting in Cocoa.

  // Personal Taste Section. I like to use a trackball interactor
  vtkSmartPointer<vtkInteractorStyleSwitch> intStyle = vtkSmartPointer<vtkInteractorStyleSwitch>::New();
  intStyle->SetCurrentStyleToTrackballCamera();
  [[self leftVTKView] getInteractor]->SetInteractorStyle(intStyle);

  // Create a cone, see the "VTK User's Guide" for details
  vtkSmartPointer<vtkConeSource> cone = vtkSmartPointer<vtkConeSource>::New();
  cone->SetHeight(3.0);
  cone->SetRadius(1.0);
  cone->SetResolution(100);

  vtkSmartPointer<vtkPolyDataMapper> coneMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  coneMapper->SetInputConnection(cone->GetOutputPort());

  vtkSmartPointer<vtkActor> coneActor = vtkSmartPointer<vtkActor>::New();
  coneActor->SetMapper(coneMapper);

  [[self leftVTKView] getRenderer]->AddActor(coneActor);

  // Tell the system that the view needs to be redrawn
  [[self leftVTKView] setNeedsDisplay:YES];
}

// ----------------------------------------------------------------------------
- (void)setupRightVTKView
{
  [[self rightVTKView] initializeVTKSupport];

  // 'smart pointers' are used because they are very similar to reference counting in Cocoa.

  // Personal Taste Section. I like to use a trackball interactor
  vtkSmartPointer<vtkInteractorStyleSwitch> intStyle = vtkSmartPointer<vtkInteractorStyleSwitch>::New();
  intStyle->SetCurrentStyleToTrackballCamera();
  [[self rightVTKView] getInteractor]->SetInteractorStyle(intStyle);

  // Create a cylinder, see the "VTK User's Guide" for details
  vtkSmartPointer<vtkCylinderSource> cylinder = vtkSmartPointer<vtkCylinderSource>::New();
  cylinder->SetResolution(100);

  vtkSmartPointer<vtkPolyDataMapper> cylinderMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  cylinderMapper->SetInputConnection(cylinder->GetOutputPort());

  vtkSmartPointer<vtkActor> cylinderActor = vtkSmartPointer<vtkActor>::New();
  cylinderActor->SetMapper(cylinderMapper);

  [[self rightVTKView] getRenderer]->AddActor(cylinderActor);

  // Tell the system that the view needs to be redrawn
  [[self rightVTKView] setNeedsDisplay:YES];
}

#pragma mark -

// ----------------------------------------------------------------------------
// Designated initializer
- (instancetype)init
{
  // Call the superclass' designated initializer, giving it the name of the nib file.
  self = [super initWithWindowNibName:@"MyWindow"];

  return self;
}

// ----------------------------------------------------------------------------
// Called once when the window is loaded.
- (void)windowDidLoad
{
  // vtk stuff
  [self setupLeftVTKView];
  [self setupRightVTKView];
}

// ----------------------------------------------------------------------------
// Called once when the window is closed.
- (void)windowWillClose:(NSNotification *)inNotification
{
  // Releases memory allocated in initializeVTKSupport.
  [[self leftVTKView] cleanUpVTKSupport];
  [[self rightVTKView] cleanUpVTKSupport];
}

#pragma mark -

// ----------------------------------------------------------------------------
- (IBAction)handleLeftButton:(id)sender
{
  // Do anything you want with the left view.
  NSBeep();

  // Here we just clean it up and remove it
  [[self leftVTKView] cleanUpVTKSupport];
  [[self leftVTKView] removeFromSuperview];
}

// ----------------------------------------------------------------------------
- (IBAction)handleRightButton:(id)sender
{
  // Do anything you want with the left view.
  NSBeep();

  // Here we just clean it up and remove it
  [[self rightVTKView] cleanUpVTKSupport];
  [[self rightVTKView] removeFromSuperview];
}

@end
