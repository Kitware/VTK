/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuartzImageWindow.h
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
// .NAME vtkQuartzImageWindow - OpenGL Imageing window
// .SECTION Description
// vtkQuartzImageWindow is a concrete implementation of the abstract
// class vtkImageWindow. vtkWin32OpenGLImageer interfaces to the standard
// OpenGL graphics library in the Windows/NT environment..

#ifndef __vtkQuartzImageWindow_h
#define __vtkQuartzImageWindow_h

#include <stdlib.h>
#include "vtkImageWindow.h"
#include "vtkMutexLock.h"

class VTK_RENDERING_EXPORT vtkQuartzImageWindow : public vtkImageWindow
{
public:
  static vtkQuartzImageWindow *New();
  vtkTypeMacro(vtkQuartzImageWindow,vtkImageWindow);
  void PrintSelf(ostream& os, vtkIndent indent);

  // output to the viewer.
  vtkQuartzImageWindow *GetOutput(){return this;};

  // Description:
  // Initialize the window for rendering.
  virtual void MakeDefaultWindow();

  // Description:
  // Swap the front and back buffers if double buffering is being used.
  void SwapBuffers();

  // Description:
  // Flush the buffer and swap if necessary
  void Frame();

  // Description:
  // Draw the contents of the window
  void Render();

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
  // Get the position in screen coordinates of the window.
  virtual int *GetPosition();

  // Description:
  // Set the name of the window. This appears at the top of the window
  // normally.
  virtual void SetWindowName(char *);
  
  //BTX
  virtual void *GetGenericDisplayId() {return (void *)this->ContextId;};
  virtual void *GetGenericWindowId()  {return (void *)this->WindowId;};
  virtual void *GetGenericParentId()  {return (void *)this->ParentId;};
  virtual void *GetGenericContext()   {return (void *)this->DeviceContext;};
  virtual void SetDisplayId(void *) {};

  // Description:
  // Get the window id.
  virtual void *GetWindowId();

  // Description:
  // Set the window id to a pre-existing window.
  virtual void  SetWindowId(void *);
  
  // Description:
  // Set the window's parent id to a pre-existing window.
  virtual void  SetParentId(void *);

  void  SetContextId(void *);	// hsr
  void  SetDeviceContext(void *);	// hsr

  // Description:
  // Set the window id of the new window once a WindowRemap is done.
  virtual void  SetNextWindowId(void *);
  //ETX

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
  // Make this windows OpenGL context the current context.
  void MakeCurrent();

  // Description:
  // These methods can be used by MFC applications 
  // to support print preview and printing, or more
  // general rendering into memory. 
  void SetupMemoryRendering(int x, int y, void *prn);
  void ResumeScreenRendering();
  void *GetMemoryDC();
  unsigned char *GetMemoryData(){return this->MemoryData;};  
  
  // Description:
  // Initialize OpenGL for this window.
  virtual void OpenGLInit();
  virtual void SetupPalette(void *hDC);
  virtual void SetupPixelFormat(void *hDC, int dwFlags, int debug, 
				int bpp=16, int zbpp=16);
  
  // Description:
  // Clean up device contexts, rendering contexts, etc.
  void Clean();

protected:
  vtkQuartzImageWindow();
  ~vtkQuartzImageWindow();
  vtkQuartzImageWindow(const vtkQuartzImageWindow&) {};
  void operator=(const vtkQuartzImageWindow&) {};

  void      *ApplicationInstance;
  void      *Palette;
  void      *OldPalette;
  void      *ContextId;
  void      *DeviceContext;
  void      *WindowId;
  void      *ParentId;
  void      *NextWindowId;
  int       OwnWindow;
  int       ScreenSize[2];

  // the following is used to support rendering into memory
  void *MemoryDataHeader;
  void *MemoryBuffer;
  unsigned char *MemoryData;	// the data in the DIBSection
  void *MemoryHdc;

  int ScreenMapped;
  int ScreenWindowSize[2];
  void *ScreenDeviceContext;
  int ScreenDoubleBuffer;
  void *ScreenContextId;

};


#endif

