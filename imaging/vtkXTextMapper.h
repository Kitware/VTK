
#ifndef __vtkXTextMapper_h
#define __vtkXTextMapper_h

#include "vtkMapper2D.h"
#include "vtkWindow.h"
#include "vtkViewport.h"
#include "vtkActor2D.h"
#include "vtkProperty2D.h"
#include "vtkTextMapper.h"

#include        <X11/Xlib.h>
#include        <X11/Xutil.h>
#include        <X11/cursorfont.h>
#include        <X11/X.h>
#include        <X11/keysym.h>


class VTK_EXPORT vtkXTextMapper : public vtkTextMapper
{
public:

  vtkXTextMapper();
  ~vtkXTextMapper();

  // Description:
  // Set the font size used by the mapper.  If the font size is 
  // available, the code will use the nearest available size.
  void SetFontSize(int size);

  static vtkXTextMapper *New() {return new vtkXTextMapper;};

  // Description:
  // Return the X compositing mode being used.
  int GetCompositingMode(vtkActor2D* actor);

  // Description:
  // Draw the text to the screen.
  void Render(vtkViewport* viewport, vtkActor2D* actor);

protected:

};




#endif



