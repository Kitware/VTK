#ifndef __vtkWin32OffscreenRenderWindow_h
#define __vtkWin32OffscreenRenderWindow_h

#include "vtkWin32OpenGLRenderWindow.h"

class VTK_EXPORT vtkWin32OffscreenRenderWindow : public vtkWin32OpenGLRenderWindow
{
public:
  vtkWin32OffscreenRenderWindow();
  ~vtkWin32OffscreenRenderWindow();

  static vtkWin32OffscreenRenderWindow *New() { return new vtkWin32OffscreenRenderWindow; }
  const char *GetClassName() { return "vtkWin32OffscreenRenderWindow"; }
  void PrintSelf(ostream &os, vtkIndent indent);
  
  virtual void Frame();
  virtual void WindowInitialize();
  virtual void SetFullScreen(int) {} // no meaning
  virtual void SetPosition(int,int) {} // no meaning for offscreen window
  virtual int *GetScreenSize() { return NULL; }
  virtual int *GetPosition() { return NULL; }
  void SetSize(int, int);
  int *GetSize();
  
  virtual void *GetGenericDisplayId() {return NULL;};
  virtual void *GetGenericWindowId()  {return NULL;};
  virtual void *GetGenericParentId()  {return NULL;};
  virtual void SetDisplayId(void *) {};
  
  virtual HWND  GetWindowId() { return NULL; }
  virtual void  SetWindowId(HWND) {}
  virtual void  SetParentId(HWND) {}
  virtual void  SetNextWindowId(HWND) {}
  
  virtual  int GetEventPending() { return 0; }
//BTX
protected:
  HBITMAP MhBitmap, MhOldBitmap;
  int MBpp, MZBpp;
  
  // overrides
  virtual void Clean();
  virtual void WindowRemap(void) {} // not used
  virtual void PrefFullScreen(void) {} // not used
//ETX
};

#endif // __vtkWin32OffscreenRenderWindow_h
