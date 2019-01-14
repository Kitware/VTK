#import <Cocoa/Cocoa.h>

#import "vtkCocoaGLView.h"

#include "vtkRenderer.h"

@interface BasicVTKView : vtkCocoaGLView

// Create the vtkRenderer, vtkRenderWindow, and vtkRenderWindowInteractor.
// initializeVTKSupport/cleanUpVTKSupport must be balanced.
- (void)initializeVTKSupport;

// Destroy the vtkRenderer, vtkRenderWindow, and vtkRenderWindowInteractor.
// initializeVTKSupport/cleanUpVTKSupport must be balanced.
- (void)cleanUpVTKSupport;

// Accessors for the vtkRenderer.
- (/*nullable*/ vtkRenderer*)getRenderer;
- (void)setRenderer:(/*nullable*/ vtkRenderer*)theRenderer;

@end
