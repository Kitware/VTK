/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWindow.h
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
// .NAME vtkWindow - window superclass for ImageWindow and RenderWindow
// .SECTION Description
// vtkWindow is an abstract object to specify the behavior of a
// rendering or imaging window. It contains vtkViewports.

// .SECTION see also
// vtkImageWindow vtkRenderWindow vtkViewport

#ifndef __vtkWindow_h
#define __vtkWindow_h

#include "vtkObject.h"
#include <stdio.h>

class VTK_COMMON_EXPORT vtkWindow : public vtkObject
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
  // Set/Get the size of the window in screen coordinates.
  virtual int *GetSize();
  virtual void SetSize(int,int);
  virtual void SetSize(int a[2]);

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
  virtual void SetWindowName(char * );

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
  virtual unsigned char *GetPixelData(int, int, int, int, int) {
    return (unsigned char *)NULL;};

  // Description:
  // Return a best estimate to the dots per inch of the display
  // device being rendered (or printed).
  vtkGetMacro(DPI,int);
  vtkSetClampMacro(DPI,int,1,3000);
  
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
  virtual void MakeCurrent() {};

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
private:
  vtkWindow(const vtkWindow&);  // Not implemented.
  void operator=(const vtkWindow&);  // Not implemented.
};

#endif


