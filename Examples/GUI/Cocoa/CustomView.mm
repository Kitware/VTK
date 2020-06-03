#import "CustomView.h"

#import "CustomLayer.h"

#import "vtkCocoaRenderWindow.h"
#import "vtkCocoaRenderWindowInteractor.h"
#import "vtkRenderer.h"

@implementation CustomView

// ----------------------------------------------------------------------------
// Designated initializer
- (instancetype)initWithFrame:(NSRect)frameRect
{
  self = [super initWithFrame:frameRect];
  if (self)
  {
    // nothing to do... add something if you need to
  }

  return self;
}

// ----------------------------------------------------------------------------
// Designated initializer
- (nullable instancetype)initWithCoder:(NSCoder*)coder
{
  self = [super initWithCoder:coder];
  if (self)
  {
    // nothing to do... add something if you need to
  }

  return self;
}

#pragma mark -

// ----------------------------------------------------------------------------
- (void)initializeVTKSupport
{
  // The usual vtk object creation.
  vtkRenderer* ren = vtkRenderer::New();
  vtkRenderWindow* renWin = vtkRenderWindow::New();
  vtkRenderWindowInteractor* renWinInt = vtkRenderWindowInteractor::New();

  // The cast should never fail, but we do it anyway, as
  // it's more correct to do so.
  vtkCocoaRenderWindow* cocoaRenWin = vtkCocoaRenderWindow::SafeDownCast(renWin);

  if (ren && cocoaRenWin && renWinInt)
  {
    //
    [self setRenderer:ren];
    [self setRenderWindow:cocoaRenWin];
    [self setRenderWindowInteractor:renWinInt];

    // This is special to our usage of vtk.  To prevent vtk
    // from creating an NSWindow and NSView automatically (its
    // default behaviour) we tell vtk that they exist already.
    // The APIs names are a bit misleading, due to the cross
    // platform nature of vtk, but this usage is correct.
    NSWindow* parentWindow = [self window];
    assert(parentWindow);
    cocoaRenWin->SetRootWindow((__bridge void*)parentWindow);
    cocoaRenWin->SetWindowId((__bridge void*)self);

    // Because we want our rendering to happen in our CAOpenGLLayer subclass (CustomLayer),
    // instruct vtk to not associate the NSOpenGLContext it creates with our NSView.
    cocoaRenWin->SetConnectContextToNSView(false);

    // The usual vtk connections.
    cocoaRenWin->AddRenderer(ren);
    renWinInt->SetRenderWindow(cocoaRenWin);

    // Initialize now so we are ready when the OS asks us to draw.
    if (!renWinInt->GetInitialized())
    {
      renWinInt->Initialize();
    }
  }
}

// ----------------------------------------------------------------------------
- (void)cleanUpVTKSupport
{
  vtkRenderer* ren = [self renderer];
  vtkRenderWindow* renWin = [self renderWindow];
  vtkRenderWindowInteractor* renWinInt = [self renderWindowInteractor];

  if (ren)
  {
    ren->Delete();
  }
  if (renWin)
  {
    renWin->Delete();
  }
  if (renWinInt)
  {
    renWinInt->Delete();
  }

  [self setRenderer:nil];
  [self setRenderWindow:nil];
  [self setRenderWindowInteractor:nil];
}

#pragma mark -

// ----------------------------------------------------------------------------
- (void)initializeLayerSupport
{
  // Make sure we don't do this twice.
  assert([[self layer] isKindOfClass:[CustomLayer class]] == NO);

  NSRect viewBounds = [self bounds];

  // Create the CustomLayer.
  CustomLayer* layer = [CustomLayer layer];
  [layer setName:@"CustomLayer"];
  [layer setAnchorPoint:CGPointZero];
  [layer setPosition:CGPointZero];
  [layer setBounds:viewBounds];
  [layer setMasksToBounds:YES];
  [layer setNeedsDisplayOnBoundsChange:YES];
  [layer setOpaque:YES];
  [layer setCustomView:self];

  // If the view has been connected to a window use the window's backing scale factor as its content
  // scale.
  NSWindow* window = [self window];
  if (window)
  {
    double backingScaleFactor = [window backingScaleFactor];
    [layer setContentsScale:backingScaleFactor];
    [self modifyDPIForBackingScaleFactor:backingScaleFactor];
  }

  // Newly created layers are generally invalidated after creation, and so must be told to redraw.
  [layer setNeedsDisplay];

  // Make us a layer-hosting (not layer-backed) view.
  [self setLayer:layer];
  [self setWantsLayer:YES];
}

#pragma mark - Backing Property Change

// ---------------------------------------------------------------------------------------------------------------
- (void)modifyDPIForBackingScaleFactor:(double)backingScaleFactor
{
  vtkCocoaRenderWindow* renderWindow = [self renderWindow];
  if (renderWindow)
  {
    // DPI has been hardcoded to 72 but in order to cater for text rendering discrepancies with
    // vtkTextActors for instance we must adjust the scaling factor per NSWindow.
    renderWindow->SetDPI((int)lround(72.0 * backingScaleFactor));
  }
}

// ---------------------------------------------------------------------------------------------------------------
- (void)viewWillMoveToWindow:(nullable NSWindow*)inNewWindow
{
  [super viewWillMoveToWindow:inNewWindow];

  if (inNewWindow)
  {
    CGFloat backingScaleFactor = [inNewWindow backingScaleFactor];
    assert(backingScaleFactor >= 1.0);

    // If the layer exists set its content scale to the window's backing scale factor otherwise it
    // will eventually get set when the layer is created (see 'initializeLayerSupport').
    [[self layer] setContentsScale:backingScaleFactor];

    // Update the rendering DPI per window.
    [self modifyDPIForBackingScaleFactor:backingScaleFactor];
  }
}

// ---------------------------------------------------------------------------------------------------------------
- (void)viewDidChangeBackingProperties
{
  [super viewDidChangeBackingProperties];

  NSWindow* window = [self window];
  if (window)
  {
    CGFloat backingScaleFactor = [[self window] backingScaleFactor];
    assert(backingScaleFactor >= 1.0);

    // If the layer exists set its content scale to the window's backing scale factor otherwise when
    // it's ready and the backing property changes (i.e moving between screens that differ in
    // resolution) it will be set.
    [[self layer] setContentsScale:backingScaleFactor];

    // Update the rendering DPI per window.
    [self modifyDPIForBackingScaleFactor:backingScaleFactor];
  }
}

@end
