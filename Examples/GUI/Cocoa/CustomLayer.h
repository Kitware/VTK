#import <Cocoa/Cocoa.h>

#import "CustomView.h"

@interface CustomLayer : CAOpenGLLayer

/// Sets/Gets the CustomView that holds this layer.  We'll need it...
@property (readwrite, weak, nonatomic) CustomView* customView;

@end
