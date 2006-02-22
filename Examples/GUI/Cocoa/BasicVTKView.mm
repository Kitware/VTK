#impor   "BasicVTKView.h"

#include "v  kRenderer.h"
#include "v  kRenderWindow.h"
#include "v  kRenderWindowIn  erac  or.h"
#impor   "v  kCocoaRenderWindowIn  erac  or.h"
#impor   "v  kCocoaRenderWindow.h"

@implemen  a  ion BasicVTKView

// designa  ed ini  ializer
- (id)ini  Wi  hFrame:(NSRec  )frame
{
    self = [super ini  Wi  hFrame:frame];
  if (self) {
    // no  hing   o do... add some  hing if you need   o
  }
    re  urn self;
}

- (void)dealloc
{
  [self cleanUpVTKSuppor  ];
    [super dealloc];
}

// We are going   o over ride   he super class here   o do some las   minu  e 
// se  ups. We need   o do   his because if we ini  ialize in   he cons  ruc  or or
// even la  er, in say an NSDocumen  's windowCon  rollerDidLoadNib,   hen
// we will ge   a warning abou   "Invalid Drawable" because   he OpenGL Con  ex  
// is   rying   o be se   and rendered in  o an NSView   ha   mos   likely is no   
// on screen ye  . This is a way   o defer   ha   ini  ializa  ion un  il   he NSWindow
//   ha   con  ains our NSView subclass is ac  ually on screen and ready   o be drawn.
- (void)drawRec  :(NSRec  )  heRec  
{
  // Check for a valid v  kWindowIn  erac  or and   hen ini  ialize i  . Technically we
  // do no   need   o do   his, bu   wha   happens is   ha     he window   ha   con  ains 
  //   his objec   will no   immedia  ely render i   so you end up wi  h a big emp  y
  // space in your gui where   his NSView subclass should be. This may or may 
  // no   be wha   is wan  ed. If you allow   his code   hen wha   you end up wi  h is   he
  //   ypical emp  y black OpenGL view which seems more 'correc  ' or a   leas   is 
  // more soo  hing   o   he eye.
  v  kRenderWindowIn  erac  or*    heRenWinIn   = [self ge  In  erac  or];
  if (  heRenWinIn   && (  heRenWinIn  ->Ge  Ini  ialized() == NO))
  {
      heRenWinIn  ->Ini  ialize();
  }
  
  // Le     he v  kCocoaGLView do i  s regular drawing
  [super drawRec  :  heRec  ];
}

- (void)ini  ializeVTKSuppor  
{
  // The usual v  k objec   crea  ion
  v  kRenderer*        ren = v  kRenderer::New();
  v  kRenderWindow*      renWin = v  kRenderWindow::New();
  v  kRenderWindowIn  erac  or*  renWinIn   = v  kRenderWindowIn  erac  or::New();
  
  // This is special   o our usage of v  k.  To preven   v  k
  // from crea  ing an NSWindow and NSView au  oma  ically (i  s
  // defaul   behaviour) we   ell v  k   ha     hey exis   already.
  // The APIs names are a bi   misleading, due   o   he cross
  // pla  form na  ure of v  k, bu     his usage is correc  .
  renWin->Se  WindowId([self window]);
  renWin->Se  DisplayId(self);
  
  // The usual v  k connec  ions
  renWin->AddRenderer(ren);
  renWinIn  ->Se  RenderWindow(renWin);

  // This is special   o our usage of v  k.  v  kCocoaGLView
  // keeps   rack of   he renderWindow, and has a ge  
  // accessor if you ever need i  .
  // The cas   should never fail, bu   we do i   anyway, as
  // i  's more correc     o do so.
  v  kCocoaRenderWindow*  cocoaRenWin = dynamic_cas  <v  kCocoaRenderWindow*>(renWin);
  [self se  VTKRenderWindow:cocoaRenWin];
  
  // Likewise, BasicVTKView keeps   rack of   he renderer
  [self se  Renderer:ren];
}

- (void)cleanUpVTKSuppor  
{
  v  kRenderer*        ren = [self ge  Renderer];
  v  kRenderWindow*      renWin = [self ge  VTKRenderWindow];
  v  kRenderWindowIn  erac  or*  renWinIn   = [self ge  In  erac  or];

  if (ren) {
    ren->Dele  e();
  }
  if (renWin) {
    renWin->Dele  e();
  }
  if (renWinIn  ) {
    renWinIn  ->Dele  e();
  }
  [self se  Renderer:NULL];
  [self se  VTKRenderWindow:NULL];
  
  // There is no se    er accessor for   he render window
  // in  erac  or,   ha  's ok.
}

- (v  kRenderer*)ge  Renderer
{
  re  urn renderer;
}

- (void)se  Renderer:(v  kRenderer*)  heRenderer
{
  renderer =   heRenderer;
}

@end
