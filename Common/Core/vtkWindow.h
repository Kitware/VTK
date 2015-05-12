/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkWindow - window superclass for vtkRenderWindow
// .SECTION Description
// vtkWindow is an abstract object to specify the behavior of a
// rendering window.  It contains vtkViewports.

// .SECTION see also
// vtkRenderWindow vtkViewport

#ifndef vtkWindow_h
#define vtkWindow_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkUnsignedCharArray;

class VTKCOMMONCORE_EXPORT vtkWindow : public vtkObject
{
public:
  vtkTypeMacro(vtkWindow,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // These are window system independent methods that are used
  // to help interface vtkWindow to native windowing systems.
  virtual void SetDisplayId(void *) = 0;
  virtual void SetWindowId(void *)  = 0;
  virtual void SetParentId(void *)  = 0;
  virtual void *GetGenericDisplayId() = 0;
  virtual void *GetGenericWindowId()  = 0;
  virtual void *GetGenericParentId()  = 0;
  virtual void *GetGenericContext()   = 0;
  virtual void *GetGenericDrawable()  = 0;
  virtual void SetWindowInfo(char *) = 0;
  virtual void SetParentInfo(char *) = 0;

  // Description:
  // Set/Get the position in screen coordinates of the rendering window.
  virtual int *GetPosition();
  virtual void SetPosition(int,int);
  virtual void SetPosition(int a[2]);

  // Description:
  // Set/Get the size of the window in screen coordinates in pixels.
  virtual int *GetSize();
  virtual void SetSize(int,int);
  virtual void SetSize(int a[2]);

  // Description:
  // GetSize() returns the size * this->TileScale, whereas this method returns
  // the size without multiplying with the tile scale.
  int *GetActualSize();

  // Description:
  // Get the current size of the screen in pixels.
  virtual int     *GetScreenSize() = 0;

  // Description:
  // Keep track of whether the rendering window has been mapped to screen.
  vtkSetMacro(Mapped,int);
  vtkGetMacro(Mapped,int);
  vtkBooleanMacro(Mapped,int);

  // Description:
  // Turn on/off erasing the screen between images. This allows multiple
  // exposure sequences if turned on. You will need to turn double
  // buffering off or make use of the SwapBuffers methods to prevent
  // you from swapping buffers between exposures.
  vtkSetMacro(Erase,int);
  vtkGetMacro(Erase,int);
  vtkBooleanMacro(Erase,int);

  // Description:
  // Keep track of whether double buffering is on or off
  vtkSetMacro(DoubleBuffer,int);
  vtkGetMacro(DoubleBuffer,int);
  vtkBooleanMacro(DoubleBuffer,int);

  // Description:
  // Get name of rendering window
  vtkGetStringMacro(WindowName);
  vtkSetStringMacro(WindowName);

  // Description:
  // Ask each viewport owned by this Window to render its image and
  // synchronize this process.
  virtual void Render() = 0;

  // Description:
  // Get the pixel data of an image, transmitted as RGBRGBRGB. The
  // front argument indicates if the front buffer should be used or the back
  // buffer. It is the caller's responsibility to delete the resulting
  // array. It is very important to realize that the memory in this array
  // is organized from the bottom of the window to the top. The origin
  // of the screen is in the lower left corner. The y axis increases as
  // you go up the screen. So the storage of pixels is from left to right
  // and from bottom to top.
  // (x,y) is any corner of the rectangle. (x2,y2) is its opposite corner on
  // the diagonal.
  virtual unsigned char *GetPixelData(int x, int y, int x2, int y2,
                                      int front) = 0;
  virtual int GetPixelData(int x, int y, int x2, int y2, int front,
                           vtkUnsignedCharArray *data) = 0;

  // Description:
  // Return a best estimate to the dots per inch of the display
  // device being rendered (or printed).
  vtkGetMacro(DPI,int);
  vtkSetClampMacro(DPI,int,1,VTK_INT_MAX);

  // Description:
  // Attempt to detect and set the DPI of the display device by querying the
  // system. Note that this is not supported on all backends, and this method
  // will return false if the DPI could not be detected. Use GetDPI() to
  // inspect the detected value.
  virtual bool DetectDPI() { return false; }

  // Description:
  // Create a window in memory instead of on the screen. This may not be
  // supported for every type of window and on some windows you may need to
  // invoke this prior to the first render.
  vtkSetMacro(OffScreenRendering,int);
  vtkGetMacro(OffScreenRendering,int);
  vtkBooleanMacro(OffScreenRendering,int);

  // Description:
  // Make the window current. May be overridden in subclasses to do
  // for example a glXMakeCurrent or a wglMakeCurrent.
  virtual void MakeCurrent() {}

  // Description:
  // These methods are used by vtkWindowToImageFilter to tell a VTK window
  // to simulate a larger window by tiling. For 3D geometry these methods
  // have no impact. It is just in handling annotation that this information
  // must be available to the mappers and the coordinate calculations.
  vtkSetVector2Macro(TileScale,int);
  vtkGetVector2Macro(TileScale,int);
  void SetTileScale(int s) {this->SetTileScale(s,s);}
  vtkSetVector4Macro(TileViewport,double);
  vtkGetVector4Macro(TileViewport,double);

protected:
  int OffScreenRendering;
  vtkWindow();
  ~vtkWindow();

  char *WindowName;
  int Size[2];
  int Position[2];
  int Mapped;
  int Erase;
  int DoubleBuffer;
  int DPI;

  double TileViewport[4];
  int    TileSize[2];
  int    TileScale[2];

private:
  vtkWindow(const vtkWindow&);  // Not implemented.
  void operator=(const vtkWindow&);  // Not implemented.
};

#endif


