
#ifndef __vtkXImageWindow_h
#define __vtkXImageWindow_h

#include 	<X11/Xlib.h>
#include 	<X11/Xutil.h>
#include 	<X11/cursorfont.h>
#include 	<X11/X.h>
#include 	<X11/keysym.h>
#include    	<fstream.h>
#include 	<stdlib.h>
#include 	<iostream.h>

#include        "vtkImageWindow.h"

class VTK_EXPORT vtkXImageWindow : public vtkImageWindow {
public:
  vtkXImageWindow();
  ~vtkXImageWindow();

  static vtkXImageWindow *New() {return new vtkXImageWindow;};
  const char *GetClassName() {return "vtkXImageWindow";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // output to the viewer.
  vtkXImageWindow *GetOutput(){return this;};
  
  // Description:
  // Gets the number of colors in the pseudo color map.
  vtkGetMacro(NumberOfColors,int);
  
  // Description:
  // Gets the windows depth. For the templated function.
  vtkGetMacro(VisualDepth,int);
  
  // Description:
  // Gets the windows visual class. For the templated function.
  vtkGetMacro(VisualClass,int);
  
  // Description:
  // These are here for using a tk window.
  Window   GetParentId();
  void     SetParentId(Window);
  void     SetParentId(void *);
  void     SetDisplayId(Display *);
  void     SetDisplayId(void *);
  void     SetWindowId(Window);
  void     SetWindowId(void *);
  Window   GetWindowId();
  Display *GetDisplayId();
  GC       GetGC();
  int     *GetPosition();
  void     SetPosition(int,int);

  int     *GetSize();
  void     SetSize(int x, int y);

  // Description:
  // The GetGenericXXX functions are necessary to draw into
  // a VTKWindow.
  void *GetGenericDisplayId() {return (void*) this->DisplayId;};
  void *GetGenericWindowId() {return (void*) this->WindowId;};
  void *GetGenericParentId() {return (void*) this->ParentId;};
  void *GetGenericContext() {return (void*) this->Gc;};
  void *GetGenericDrawable();

  // Description:
  // Swaps the 2D drawing buffers.  The user should not need to 
  // use this call.  To invoke double buffering, call DoubleBufferOn
  void SwapBuffers();

  // Description:
  // Determine the desired depth of the window.
  virtual int      GetDesiredDepth();
  
  // Description:
  // Determine the desired colormap of the window.
  virtual Colormap GetDesiredColormap();

  // Description:
  // Determine the desired visual for the window
  virtual Visual  *GetDesiredVisual();

  // Description:
  // Return the id of the visual we are using
  Visual *GetVisualId() {return VisualId;};

  // Description:
  // Set the background color of the window.
  void SetBackgroundColor(float r, float g, float b);

  // Description:
  // Erase the contents of the window.
  void EraseWindow();

  // Description:
  // Get the pixel data of an image, transmitted as RGBRGBRGB. 
  // It is the caller's responsibility to delete the resulting 
  // array. It is very important to realize that the memory in this array
  // is organized from the bottom of the window to the top. The origin
  // of the screen is in the lower left corner. The y axis increases as
  // you go up the screen. So the storage of pixels is from left to right
  // and from bottom to top.  To maintain the same prototype as in 
  // vtkRenderWindow, the last argument is provided, but currently not used.
  unsigned char *GetPixelData(int x1, int y1, int x2, int y2, int);

protected:
  // X stuff
  Window               ParentId;
  Window	           WindowId;
  Display             *DisplayId;
  Visual              *VisualId;
  int                  VisualDepth;
  int                  VisualClass;
  Colormap             ColorMap;
  GC                   Gc;
  Pixmap               IconPixmap;
  XEvent               report;
  int	               Offset;
  XColor               Colors[256];
  int                  NumberOfColors;
  Pixmap               Drawable;
  
  void MakeDefaultWindow();
  void GetDefaultVisualInfo(XVisualInfo *info);
  Colormap MakeColorMap(Visual *visual);
  void AllocateDirectColorMap();
};

#endif





