#import "vtkQuartzBridge.h"
#import <Cocoa/Cocoa.h>
#import <stdio.h>
#import "vtkQuartzWindowController.h"
#import "vtkQuartzGLView.h"
#import "vtkQuartzWindow.h"

void QBStartTimer(float seconds) {
    [NSEvent startPeriodicEventsAfterDelay:seconds withPeriod:seconds];
}

void *QBcreateAutoReleasePool() {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    return pool;
}

int QBreleaseAutoreleasePool(void *objcPool) {
    NSAutoreleasePool *thePool;

    thePool = (NSAutoreleasePool *)objcPool;
    [thePool release];
}

void QBmakeApplication() {
    [NSApplication sharedApplication];
}

void QBrunApplication() {
    [NSApp run];
}

void QBDestroyWindow(void *objcController) {
    [(vtkQuartzWindowController *)objcController close];
}

void QBKillApplication(void *objcController) {
    [NSApp terminate:(vtkQuartzWindowController *)objcController];
}

void QBSetWindowSize(void *objcController, int posX, int posY, int sizeX, int sizeY) {
    NSRect sizeRect = NSMakeRect(posX, posY, sizeX, sizeY);
    
    [[(vtkQuartzWindowController *)objcController getvtkQuartzWindow] setFrame:sizeRect display:YES];
}


void *QBmakeWindow() {
    vtkQuartzWindowController *MyvtkQuartzWindowController = [[vtkQuartzWindowController alloc] retain];
    [MyvtkQuartzWindowController init];

    [MyvtkQuartzWindowController setVTKRenderWindow:0];
    [MyvtkQuartzWindowController setVTKRenderWindowInteractor:0];
    return MyvtkQuartzWindowController;
}

void QBSetVTKRenderWindow(void *objcController, void *theVTKRenderWindow) {
    [(vtkQuartzWindowController *)objcController setVTKRenderWindow:theVTKRenderWindow];
}

void QBSetVTKRenderWindowInteractor(void *objcController, void *theVTKRenderWindowInteractor) {
    [(vtkQuartzWindowController *)objcController setVTKRenderWindowInteractor:theVTKRenderWindowInteractor];
}

void QBstartTimer(void *objcController) {
//    [[(DemoController *)objcController window] startTimer];
}

void QBstopTimer(void *objcController) {
//    [[(DemoController *)objcController window] stopTimer];
}
    
void QBmakeCurrentContext(void *objcController) {
    [(vtkQuartzWindowController *)objcController makeCurrentContext];
}

void QBsendDisplay(void *objcController) {
    [[(vtkQuartzWindowController *)objcController getvtkQuartzGLView] display];
}

float QBGetWindowWidth(void *objcController) {
    return [[(vtkQuartzWindowController *)objcController getvtkQuartzWindow] frame].size.width;
}

float QBGetWindowHeight(void *objcController) {
    return [[(vtkQuartzWindowController *)objcController getvtkQuartzWindow] frame].size.height;
}

float QBGetWindowXPosition(void *objcController) {
    return [[(vtkQuartzWindowController *)objcController getvtkQuartzWindow] frame].origin.x;
}

float QBGetWindowYPosition(void *objcController) {
    return [[(vtkQuartzWindowController *)objcController getvtkQuartzWindow] frame].origin.y;
}
