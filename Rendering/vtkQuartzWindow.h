#import <Cocoa/Cocoa.h>

@class vtkQuartzWindowController;

@interface vtkQuartzWindow : NSWindow
{
    @private
    IBOutlet vtkQuartzWindowController *controller;
    NSTimer *MyNSTimer;
}

// DemoController accessor and convenience
- (void)setvtkQuartzWindowController:(vtkQuartzWindowController *)theController;
- (vtkQuartzWindowController *)getvtkQuartzWindowController;

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)proposedFrameSize;
- (BOOL)windowShouldZoom:(NSWindow *)sender toFrame:(NSRect)newFrame;
- (void)close; //close your face!


@end