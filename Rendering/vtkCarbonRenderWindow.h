/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCarbonRenderWindow.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCarbonRenderWindow - Carbon OpenGL rendering window
// .SECTION Description
// vtkCarbonRenderWindow is a concrete implementation of the abstract
// class vtkOpenGLRenderWindow. vtkCarbonRenderWindow interfaces to the
// OpenGL graphics library using the Carbon API on Mac OSX.

#ifndef __vtkCarbonRenderWindow_h
#define __vtkCarbonRenderWindow_h

#include <stdlib.h>
#include "vtkOpenGLRenderWindow.h"
#include <Carbon/Carbon.h>
#include <OpenGL/gl.h>
#include <AGL/agl.h>
#endif

class vtkIdList;

class VTK_RENDERING_EXPORT vtkCarbonRenderWindow : public vtkOpenGLRenderWindow
{
public:
  static vtkCarbonRenderWindow *New();
  vtkTypeRevisionMacro(vtkCarbonRenderWindow,vtkRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Begin the rendering process.
  void Start(void);

  // Description:
  // End the rendering process and display the image.
  void Frame(void);

  // Description:
  // Specify various window parameters.
  virtual void WindowConfigure(void);

  // Description:
  // Initialize the window for rendering.
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
  // Return the scrren size.
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
  void SetWindowInfo(void *);

  //BTX
  virtual void *GetGenericDisplayId() {return (void *)this->ContextId;};
  virtual void *GetGenericWindowId()  {return (void *)this->WindowId;};
  virtual void *GetGenericParentId()  {return (void *)this->ParentId;};
  virtual AGLContext GetContextId()   {return this->ContextId;};
  virtual void *GetGenericContext()   {return (void *)this->DeviceContext;};
  virtual void SetDisplayId(void *) {};

  virtual void* GetGenericDrawable()
    {
      vtkWarningMacro("GetGenericDrawable Method not implemented.");
      return 0;
    }
  void SetWindowInfo(char*)
    {
      vtkWarningMacro("SetWindowInfo Method not implemented.");
    }
  void SetParentInfo(char*)
    {
      vtkWarningMacro("SetParentInfo Method not implemented.");
    }

  // Description:
  // Get the window id.
  virtual WindowPtr GetWindowId();
  void  SetWindowId(void *foo) {this->SetWindowId((WindowPtr)foo);};

  // Description:
  // Get the window id.
  virtual void SetParentId(WindowPtr);
  void  SetParentId(void *foo) {this->SetParentId((WindowPtr)foo);};
  
  // Description:
  // Set the window id to a pre-existing window.
  virtual void SetWindowId(WindowPtr);

  void  SetContextId(void *);   // hsr
  void  SetDeviceContext(void *);       // hsr

  //ETX

  // supply base class virtual function
  vtkSetMacro(MultiSamples,int);
  vtkGetMacro(MultiSamples,int);

  // Description:
  // Update system if needed due to stereo rendering.
  virtual void StereoUpdate();
  
  // Description:
  // Prescribe that the window be created in a stereo-capable mode. This
  // method must be called before the window is realized. This method
  // overrrides the superclass method since this class can actually check
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
//  void *GetMemoryDC();
//  unsigned char *GetMemoryData(){return this->MemoryData;};  
  
  // Description:
  // Initialize OpenGL for this window.
  virtual void SetupPalette(void *hDC);
  virtual void SetupPixelFormat(void *hDC, void *dwFlags, int debug, 
                                int bpp=16, int zbpp=16);
  
  // Description:
  // Clean up device contexts, rendering contexts, etc.
  void Clean();

  // Description:
  // Get the size of the depth buffer.
  int GetDepthBufferSize();

  // Description:
  // Hide or Show the mouse cursor, it is nice to be able to hide the
  // default cursor if you want VTK to display a 3D cursor instead.
  void HideCursor();
  void ShowCursor();
  
  void UpdateSizeAndPosition(int xPos, int yPos, int xSize, int ySize);

  
protected:
  vtkCarbonRenderWindow();
  ~vtkCarbonRenderWindow();

  int ApplicationInitialized; // Toolboxen initialized?
  Boolean fAcceleratedMust;   // input: must renderer be accelerated?
  Boolean draggable;          // input: is the window draggable?
  GLint aglAttributes[64];    // input: pixel format attributes always required
                              //   (reset to what was actually allocated)
  SInt32 VRAM;                // input: minimum VRAM; output: actual
                              //   (if successful otherwise input)
  SInt32 textureRAM;          // input: amount of texture RAM required on card;
                              // output: same (used in allocation)
  AGLPixelFormat fmt;         // input: none; output pixel format...
  AGLContext ContextId;
  AGLDrawable DeviceContext;  // the drawable attached to a rendering context
  WindowPtr WindowId;
  WindowPtr ParentId;
  int OwnWindow;
  int ScreenSize[2];

  int ScreenMapped;
  int ScreenWindowSize[2];
  void *ScreenDeviceContext;
  int ScreenDoubleBuffer;
  void *ScreenContextId;

  int CursorHidden;

private:
  vtkCarbonRenderWindow(const vtkCarbonRenderWindow&);  // Not implemented.
  void operator=(const vtkCarbonRenderWindow&);  // Not implemented.
};

