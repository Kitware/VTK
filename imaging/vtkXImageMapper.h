#ifndef __vtkXImageMapper_h
#define __vtkXImageMapper_h

#include "vtkImageMapper.h"
#include "vtkWindow.h"
#include "vtkViewport.h"
#include "vtkImageData.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/X.h>
#include <X11/keysym.h>

class vtkActor2D;

class VTK_EXPORT vtkXImageMapper : public vtkImageMapper
{
public:
  vtkXImageMapper();
  ~vtkXImageMapper();

  static vtkXImageMapper *New() {return new vtkXImageMapper;};

  // Description:
  // Called by the Render function in vtkImageMapper.  Actually draws
  // the image to the screen.
  void RenderData(vtkViewport* viewport, vtkImageData* data, vtkActor2D* actor);

  // Description:
  // Returns the X specific compositing operator.
  int GetCompositingMode(vtkActor2D* actor);

  // Description:
  // Returns the depth of the X window
  int GetXWindowDepth(vtkWindow* window);

  // Description:
  // Returns the visual id of the window
  void GetXWindowVisualId(vtkWindow* window, Visual* visualID);

  // Description:
  // Returns the visual class of the window
  int GetXWindowVisualClass(vtkWindow* window);
 
  // Description:
  // Returns the color map used by the window
  void GetXWindowColors(vtkWindow* window, XColor colors[], int ncolors);

  // Description:
  // Returns the color masks used by the window.
  void GetXWindowColorMasks(vtkWindow *window, unsigned long *rmask,
      			    unsigned long *gmask, unsigned long *bmask);

  // Description:
  // Gets the number of colors in the pseudo color map.
  vtkGetMacro(NumberOfColors,int);

protected:

  XImage          *Image;
  unsigned char   *DataOut;
  int             DataOutSize;
  int             NumberOfColors;
};


#endif




