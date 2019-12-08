//
//  VTKView.h
//
//  Created by Alexis Girault on 4/3/17.
//

#import <GLKit/GLKit.h>

class vtkIOSRenderWindowInteractor;
class vtkRenderWindow;

@interface VTKView : GLKView

- (void)displayCoordinates:(int*)coordinates ofTouch:(CGPoint)touchPoint;
- (void)normalizedCoordinates:(double*)coordinates ofTouch:(CGPoint)touch;

@property (assign, readonly) vtkRenderWindow* renderWindow;
@property (assign, readonly) vtkIOSRenderWindowInteractor* interactor;

@end
