#ifndef __vtkWin32ImageWindow_h
#define __vtkWin32ImageWindow_h


#include 	"vtkImageWindow.h"

class VTK_EXPORT vtkWin32ImageWindow : public vtkImageWindow 
{
public:
  HINSTANCE ApplicationInstance;
  HPALETTE  Palette;
  HDC       DeviceContext;
  HWND      WindowId;
  HWND      ParentId;

  // ###
  void SwapBuffers();

  vtkWin32ImageWindow();
  ~vtkWin32ImageWindow();
  static vtkWin32ImageWindow *New() {return new vtkWin32ImageWindow;};
  const char *GetClassName() {return "vtkWin32ImageWindow";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // output to the viewer.
  vtkWin32ImageWindow *GetOutput(){return this;};
  
  //BTX
  void SetDisplayId(void *foo) {vtkDebugMacro(<<"SetDisplayID not implemented");};
  HWND GetWindowId(); 
  void SetWindowId(void* id) {this->WindowId = (HWND) id;};
  void SetParentId(void* id) {this->ParentId = (HWND) id;};
  void SetDeviceContext(void* dc) {this->DeviceContext = (HDC) dc;};
  void SetWindowId(HWND);
  void SetParentId(HWND);
  void SetDeviceContext(HDC);

  void *GetGenericDisplayId() 
        {vtkDebugMacro(<<"Display ID not implemented in Win32."); return (void*) NULL;};
  void *GetGenericWindowId() {return (void*) this->WindowId;};
  void *GetGenericParentId() {return (void*) this->ParentId;};
  void *GetGenericContext() {return (void*) this->DeviceContext;};
 

  //ETX

  void   SetSize(int,int);
  int   *GetSize();
  int   *GetPosition();
  void   SetPosition(int,int);
  void PenLineTo(int x,int y);
  void PenMoveTo(int x,int y);
  void SetPenColor(float r,float g,float b);

  // Pen for 2D graphics line drawing
  HPEN Pen;

  void SetBackgroundColor(float r, float g, float b);
  void EraseWindow();

  // Pen color for the graphics;
  COLORREF PenColor;

  // ###
  unsigned char *GetDIBPtr();
  unsigned char *GetPixelData(int x1, int y1, int x2, int y2, int);

protected:
  int OwnWindow; // do we create this window ?
  void MakeDefaultWindow();  
  // ###
  unsigned char *DIBPtr;	// the data in the DIBSection
};

#endif
