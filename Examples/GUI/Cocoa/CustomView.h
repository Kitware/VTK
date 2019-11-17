#import <Cocoa/Cocoa.h>

#import "vtkRenderWindow.h"
#import "vtkRenderer.h"
//#import "vtkRenderWindowInteractor.h"
#import "vtkCocoaRenderWindow.h"
#import "vtkCocoaRenderWindowInteractor.h"

// This is a subclass of plain old NSView.
@interface CustomView : NSView

// Create the vtkRenderer, vtkRenderWindow, and vtkRenderWindowInteractor.
// initializeVTKSupport/cleanUpVTKSupport must be balanced.
- (void)initializeVTKSupport;

// Destroy the vtkRenderer, vtkRenderWindow, and vtkRenderWindowInteractor.
// initializeVTKSupport/cleanUpVTKSupport must be balanced.
- (void)cleanUpVTKSupport;

//
- (void)initializeLayerSupport;

// Accessors.
@property (readwrite, nonatomic, nullable) vtkRenderer* renderer;
@property (readwrite, nonatomic, nullable) vtkCocoaRenderWindow* renderWindow;
@property (readwrite, nonatomic, nullable) vtkRenderWindowInteractor* renderWindowInteractor;

@end
