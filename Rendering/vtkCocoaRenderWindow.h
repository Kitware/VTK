/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCocoaRenderWindow.h
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
// .NAME vtkCocoaRenderWindow - Cocoa OpenGL rendering window
// .SECTION Description
// vtkCocoaRenderWindow is a concrete implementation of the abstract
// class vtkOpenGLRenderWindow. vtkCocoaRenderWindow interfaces to the
// OpenGL graphics library using the Cocoa API on Mac OSX.

#ifndef __vtkCocoaRenderWindow_h
#define __vtkCocoaRenderWindow_h

#include <stdlib.h>
#include "vtkOpenGLRenderWindow.h"
#include <OpenGL/gl.h>
#endif

class vtkIdList;

class VTK_RENDERING_EXPORT vtkCocoaRenderWindow : public vtkOpenGLRenderWindow
{
public:
  static vtkCocoaRenderWindow *New();
  vtkTypeRevisionMacro(vtkCocoaRenderWindow,vtkRenderWindow);
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
  virtual void *GetGenericContext()   {return (void *)this->DeviceContext;};
  virtual void SetDisplayId(void *) {};
  virtual void SetParentId(void *) 
    {
      vtkWarningMacro("Method not implemented.");
    }
  virtual void* GetGenericParentId()
    {
      vtkWarningMacro("Method not implemented.");
      return 0;
    }
  virtual void* GetGenericDrawable()
    {
      vtkWarningMacro("Method not implemented.");
      return 0;
    }
  void SetWindowInfo(char*)
    {
      vtkWarningMacro("Method not implemented.");
    }
  void SetParentInfo(char*)
    {
      vtkWarningMacro("Method not implemented.");
    }

  // Description:
  // Get the window id.
  virtual void *GetWindowId();

  // Description:
  // Set the window id to a pre-existing window.
  virtual void  SetWindowId(void *);
  
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
  virtual void OpenGLInit();
  virtual void SetupPalette(void *hDC);
  virtual void SetupPixelFormat(void *hDC, void *dwFlags, int debug, 
                                int bpp=16, int zbpp=16);
  
  // Description:
  // Clean up device contexts, rendering contexts, etc.
  void Clean();

  // Description:
  // Register a texture name with this render window
  void RegisterTextureResource (GLuint id);

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
  vtkCocoaRenderWindow();
  ~vtkCocoaRenderWindow();

  int       ApplicationInitialized; //NSApplication called?
  void     *ContextId;
  void     *DeviceContext;
  void     *WindowId;
  void     *WindowController;
  int       OwnWindow;
  int       ScreenSize[2];
  int       MultiSamples;
  vtkIdList *TextureResourceIds;

  int GetPixelData(int x,int y,int x2,int y2,int front, unsigned char* data);
  int GetZbufferData( int x1, int y1, int x2, int y2, float* z );
  int GetRGBAPixelData(int x,int y,int x2,int y2, int front, float* data);
  int GetRGBACharPixelData(int x,int y,int x2,int y2, int front,
                           unsigned char* data);

  // the following is used to support rendering into memory
//  void *MemoryDataHeader;
//  void *MemoryBuffer;
//  unsigned char *MemoryData;  // the data in the DIBSection
//  void *MemoryHdc;

  int ScreenMapped;
  int ScreenWindowSize[2];
  void *ScreenDeviceContext;
  int ScreenDoubleBuffer;
  void *ScreenContextId;

  int CursorHidden;

private:
  vtkCocoaRenderWindow(const vtkCocoaRenderWindow&);  // Not implemented.
  void operator=(const vtkCocoaRenderWindow&);  // Not implemented.
};

