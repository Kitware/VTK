#import <Cocoa/Cocoa.h>

@class BasicVTKView;

@interface MyDocument : NSDocument
{
    IBOutlet        BasicVTKView*           leftVTKView;
    IBOutlet        BasicVTKView*           rightVTKView;
}

@end
