#import "BasicVTKView.h"

#import "vtkRenderer.h"
#import "vtkRenderWindow.h"
#import "vtkRenderWindowInteractor.h"
#import "vtkCocoaRenderWindowInteractor.h"
#import "vtkCocoaRenderWindow.h"

@implementation BasicVTKView

// designated initializer
- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // nothing to do... add something if you need to
    }
    return self;
}

- (void)dealloc
{
    [self cleanUpVTKSupport];
    [super dealloc];
}

// We are going to over ride the super class here to do some last minute 
// setups. We need to do this because if we initialize in the constructor or
// even later, in say an NSDocument's windowControllerDidLoadNib, then
// we will get a warning about "Invalid Drawable" because the OpenGL Context
// is trying to be set and rendered into an NSView that most likely is not 
// on screen yet. This is a way to defer that initialization until the NSWindow
// that contains our NSView subclass is actually on screen and ready to be drawn.
- (void)drawRect:(NSRect)theRect
{
    // Check for a valid vtkWindowInteractor and then initialize it. Technically we
    // do not need to do this, but what happens is that the window that contains 
    // this object will not immediately render it so you end up with a big empty
    // space in your gui where this NSView subclass should be. This may or may 
    // not be what is wanted. If you allow this code then what you end up with is the
    // typical empty black OpenGL view which seems more 'correct' or at least is 
    // more soothing to the eye.
    vtkRenderWindowInteractor*  theRenWinInt = [self getInteractor];
    if (theRenWinInt && (theRenWinInt->GetInitialized() == NO))
    {
        theRenWinInt->Initialize();
    }
    
    // Let the vtkCocoaGLView do its regular drawing
    [super drawRect:theRect];
}

- (void)initializeVTKSupport
{
    // The usual vtk object creation
    vtkRenderer*                ren = vtkRenderer::New();
    vtkRenderWindow*            renWin = vtkRenderWindow::New();
    vtkRenderWindowInteractor*  renWinInt = vtkRenderWindowInteractor::New();
    
    // This is special to our usage of vtk.  To prevent vtk
    // from creating an NSWindow and NSView automatically (its
    // default behaviour) we tell vtk that they exist already.
    // The APIs names are a bit misleading, due to the cross
    // platform nature of vtk, but this usage is correct.
    renWin->SetWindowId([self window]);
    renWin->SetDisplayId(self);
    
    // The usual vtk connections
    renWin->AddRenderer(ren);
    renWinInt->SetRenderWindow(renWin);

    // This is special to our usage of vtk.  vtkCocoaGLView
    // keeps track of the renderWindow, and has a get
    // accessor if you ever need it.
    // The cast should never fail, but we do it anyway, as
    // it's more correct to do so.
    vtkCocoaRenderWindow*   cocoaRenWin = vtkCocoaRenderWindow::SafeDownCast(renWin);
    [self setVTKRenderWindow:cocoaRenWin];
    
    // Likewise, BasicVTKView keeps track of the renderer
    [self setRenderer:ren];
}

- (void)cleanUpVTKSupport
{
    vtkRenderer*                ren = [self getRenderer];
    vtkRenderWindow*            renWin = [self getVTKRenderWindow];
    vtkRenderWindowInteractor*  renWinInt = [self getInteractor];

    if (ren) {
        ren->Delete();
    }
    if (renWin) {
        renWin->Delete();
    }
    if (renWinInt) {
        renWinInt->Delete();
    }
    [self setRenderer:NULL];
    [self setVTKRenderWindow:NULL];
    
    // There is no setter accessor for the render window
    // interactor, that's ok.
}

- (vtkRenderer*)getRenderer
{
    return renderer;
}

- (void)setRenderer:(vtkRenderer*)theRenderer
{
    renderer = theRenderer;
}

@end
