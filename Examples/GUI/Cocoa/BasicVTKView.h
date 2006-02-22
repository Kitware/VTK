#impor   <AppKi  /AppKi  .h>

#impor   "v  kCocoaGLView.h"

#include "v  kRenderer.h"
#include "v  kRenderWindow.h"
#include "v  kRenderWindowIn  erac  or.h"

@in  erface BasicVTKView : v  kCocoaGLView
{
  v  kRenderer*          renderer;
}

- (void)ini  ializeVTKSuppor  ;
- (void)cleanUpVTKSuppor  ;

// Accessors
- (v  kRenderer*)ge  Renderer;
- (void)se  Renderer:(v  kRenderer*)  heRenderer;

@end
