/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuartzRenderWindow.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkQuartzRenderWindow - OpenGL rendering window
// .SECTION Description
// vtkQuartzRenderWindow is a concrete implementation of the abstract
// class vtkRenderWindow. vtkQuartzRenderer interfaces to the standard
// OpenGL graphics library in the Windows/NT environment..

#ifndef __vtkQuartzRenderWindow_h
#define __vtkQuartzRenderWindow_h

#include <stdlib.h>
#include "vtkRenderWindow.h"
#include "vtkMutexLock.h"
#include <OpenGL/gl.h>
#endif



//------------------------------------------------------------------------------
// A bunch of routines that can be called using C conventions which make up the
// C++ side of the ObjC-C++ bridge. (called from MyNSWindow.m)
//------------------------------------------------------------------------------
#ifdef __cplusplus
  extern "C" {
#endif

void VBDestroyWindow(void *vtkClass);
void VBResizeWindow(void *vtkClass, int xPos, int yPos, int xSize, int ySize);
void VBRedrawWindow(void *vtkClass);
#ifdef __cplusplus
  };
#endif




class vtkIdList;

class VTK_RENDERING_EXPORT vtkQuartzRenderWindow : public vtkRenderWindow
{
public:
  static vtkQuartzRenderWindow *New();
  vtkTypeMacro(vtkQuartzRenderWindow,vtkRenderWindow);
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

  // Description:
  // Get the window id.
  virtual void *GetWindowId();

  // Description:
  // Set the window id to a pre-existing window.
  virtual void  SetWindowId(void *);
  
  void  SetContextId(void *);	// hsr
  void  SetDeviceContext(void *);	// hsr

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
  // Set/Get the pixel data of an image, transmitted as RGBRGB... 
  virtual unsigned char *GetPixelData(int x,int y,int x2,int y2,int front);
  virtual void SetPixelData(int x,int y,int x2,int y2,unsigned char *,
			    int front);

  // Description:
  // Set/Get the pixel data of an image, transmitted as RGBARGBA... 
  virtual float *GetRGBAPixelData(int x,int y,int x2,int y2,int front);
  virtual void SetRGBAPixelData(int x,int y,int x2,int y2,float *,int front,
                                int blend=0);
  virtual void ReleaseRGBAPixelData(float *data);

  // Description:
  // Set/Get the zbuffer data from an image
  virtual float *GetZbufferData( int x1, int y1, int x2, int y2 );
  virtual void SetZbufferData( int x1, int y1, int x2, int y2, float *buffer );

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
  vtkQuartzRenderWindow();
  ~vtkQuartzRenderWindow();

  int       ApplicationInitialized; //NSApplication called?
  void     *ContextId;
  void     *DeviceContext;
  void     *WindowId;
  void     *WindowController;
  int       OwnWindow;
  int       ScreenSize[2];
  int       MultiSamples;
  vtkIdList *TextureResourceIds;

  // the following is used to support rendering into memory
//  void *MemoryDataHeader;
//  void *MemoryBuffer;
//  unsigned char *MemoryData;	// the data in the DIBSection
//  void *MemoryHdc;

  int ScreenMapped;
  int ScreenWindowSize[2];
  void *ScreenDeviceContext;
  int ScreenDoubleBuffer;
  void *ScreenContextId;

  int CursorHidden;

private:
  vtkQuartzRenderWindow(const vtkQuartzRenderWindow&);  // Not implemented.
  void operator=(const vtkQuartzRenderWindow&);  // Not implemented.
};

