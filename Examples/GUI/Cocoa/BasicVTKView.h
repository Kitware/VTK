#import <Cocoa/Cocoa.h>

#import "vtkCocoaGLView.h"

#import "vtkRenderer.h"

// This is a subclass of VTK's vtkCocoaGLView.
@interface BasicVTKView : vtkCocoaGLView

// Create the vtkRenderer, vtkRenderWindow, and vtkRenderWindowInteractor.
// initializeVTKSupport/cleanUpVTKSupport must be balanced.
- (void)initializeVTKSupport;

// Destroy the vtkRenderer, vtkRenderWindow, and vtkRenderWindowInteractor.
// initializeVTKSupport/cleanUpVTKSupport must be balanced.
- (void)cleanUpVTKSupport;

// Accessors for the vtkRenderer.
@property (readwrite, nonatomic, nullable, getter=getRenderer) vtkRenderer* renderer;

@end
