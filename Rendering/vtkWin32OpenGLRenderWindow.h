/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32OpenGLRenderWindow.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkWin32OpenGLRenderWindow - OpenGL rendering window
// .SECTION Description
// vtkWin32OpenGLRenderWindow is a concrete implementation of the abstract
// class vtkRenderWindow. vtkWin32OpenGLRenderer interfaces to the standard
// OpenGL graphics library in the Windows/NT environment..

#ifndef __vtkWin32OpenGLRenderWindow_h
#define __vtkWin32OpenGLRenderWindow_h

#include <stdlib.h>
#include "vtkOpenGLRenderWindow.h"
#include "vtkMutexLock.h"
#ifndef VTK_IMPLEMENT_MESA_CXX
#include <GL/gl.h>
#endif

class vtkIdList;

class VTK_EXPORT vtkWin32OpenGLRenderWindow : public vtkOpenGLRenderWindow
{
public:
  static vtkWin32OpenGLRenderWindow *New();
  vtkTypeMacro(vtkWin32OpenGLRenderWindow,vtkOpenGLRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Begin the rendering process.
  virtual void Start(void);

  // Description:
  // End the rendering process and display the image.
  void Frame(void);

  // Description:
  // Create the window
  virtual void WindowInitialize(void);

  // Description:
  // Initialize the rendering window.
  virtual void Initialize(void);

  // Description:
  // Change the window to fill the entire screen.
  virtual void SetFullScreen(int);

  // Description:
  // Remap the window.
  virtual void WindowRemap(void);

  // Description:
  // Set the preferred window size to full screen.
  virtual void PrefFullScreen(void);

  // Description:
  // Set the size of the window.
  virtual void SetSize(int,int);

  // Description:
  // Get the current size of the window.
  virtual int *GetSize();

  // Description:
  // Set the position of the window.
  virtual void SetPosition(int,int);
  
  // Description:
  // Return the screen size.
  virtual int *GetScreenSize();

  // Description:
  // Get the position in screen coordinates of the window.
  virtual int *GetPosition();

  // Description:
  // Set the name of the window. This appears at the top of the window
  // normally.
  virtual void SetWindowName(char *);
  
  // Description:
  // Set this RenderWindow's window id to a pre-existing window.
  void SetWindowInfo(char *);

  // Description:
  // Sets the HWND id of the window that WILL BE created.
  void SetParentInfo(char *);

  //BTX
  virtual void *GetGenericDisplayId() {return (void *)this->ContextId;};
  virtual void *GetGenericWindowId()  {return (void *)this->WindowId;};
  virtual void *GetGenericParentId()  {return (void *)this->ParentId;};
  virtual void *GetGenericContext()   {return (void *)this->DeviceContext;};
  virtual void SetDisplayId(void *) {};

  // Description:
  // Get the window id.
  virtual HWND  GetWindowId();
  void  SetWindowId(void *foo) {this->SetWindowId((HWND)foo);};

  // Description:
  // Set the window id to a pre-existing window.
  virtual void  SetWindowId(HWND);
  
  // Description:
  // Set the window's parent id to a pre-existing window.
  virtual void  SetParentId(HWND);
  void  SetParentId(void *foo) {this->SetParentId((HWND)foo);};

  void  SetContextId(HGLRC);	// hsr
  void  SetDeviceContext(HDC);	// hsr

  // Description:
  // Set the window id of the new window once a WindowRemap is done.
  virtual void  SetNextWindowId(HWND);
  //ETX

  // Description:
  // Prescribe that the window be created in a stereo-capable mode. This
  // method must be called before the window is realized. This method
  // overrides the superclass method since this class can actually check
  // whether the window has been realized yet.
  virtual void SetStereoCapableWindow(int capable);

  // Description:
  // Make this windows OpenGL context the current context.
  void MakeCurrent();

  // Description:
  // Check to see if an event is pending for this window.
  // This is a useful check to abort a long render.
  virtual  int GetEventPending();

  // Description:
  // These methods can be used by MFC applications 
  // to support print preview and printing, or more
  // general rendering into memory. 
  void SetupMemoryRendering(int x, int y, HDC prn);
  void SetupMemoryRendering(HBITMAP hbmp);
  void ResumeScreenRendering();
  HDC GetMemoryDC();
  unsigned char *GetMemoryData(){return this->MemoryData;};  

  // Description:
  // Initialize OpenGL for this window.
  virtual void SetupPalette(HDC hDC);
  virtual void SetupPixelFormat(HDC hDC, DWORD dwFlags, int debug, 
				int bpp=16, int zbpp=16);
  
  // Description:
  // Clean up device contexts, rendering contexts, etc.
  void Clean();

  // Description:
  // Hide or Show the mouse cursor, it is nice to be able to hide the
  // default cursor if you want VTK to display a 3D cursor instead.
  void HideCursor();
  void ShowCursor();

  // Description:
  // Override the default implementation so that we can actively switch between
  // on and off screen rendering.
  virtual void SetOffScreenRendering(int offscreen);

protected:
  vtkWin32OpenGLRenderWindow();
  ~vtkWin32OpenGLRenderWindow();
  vtkWin32OpenGLRenderWindow(const vtkWin32OpenGLRenderWindow&);
  void operator=(const vtkWin32OpenGLRenderWindow&);

  HINSTANCE ApplicationInstance;
  HPALETTE  Palette;
  HPALETTE  OldPalette;
  HGLRC     ContextId;
  HDC       DeviceContext;
  BOOL      MFChandledWindow;
  HWND      WindowId;
  HWND      ParentId;
  HWND      NextWindowId;
  int       OwnWindow;
  int       ScreenSize[2];

  // the following is used to support rendering into memory
  BITMAPINFO MemoryDataHeader;
  HBITMAP MemoryBuffer;
  unsigned char *MemoryData;	// the data in the DIBSection
  HDC MemoryHdc;

  int ScreenMapped;
  int ScreenWindowSize[2];
  HDC ScreenDeviceContext;
  int ScreenDoubleBuffer;
  HGLRC ScreenContextId;

  //BTX
  // message handler
  virtual LRESULT MessageProc(HWND hWnd, UINT message, 
			      WPARAM wParam, LPARAM lParam);

  static LRESULT APIENTRY WndProc(HWND hWnd, UINT message, 
				  WPARAM wParam, LPARAM lParam);
  //ETX
  int CursorHidden;

  void ResizeWhileOffscreen(int xsize, int ysize);
  void CreateAWindow(int x, int y, int width, int height);
  void InitializeApplication();
  void CleanUpOffScreenRendering();
  void CreateOffScreenDC(int xsize, int ysize, HDC aHdc);
  void CreateOffScreenDC(HBITMAP hbmp, HDC aHdc);
};


#endif

