/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32OffscreenRenderWindow.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    to Horst Schreiber for developing this MFC code

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

#ifndef __vtkWin32OffscreenRenderWindow_h
#define __vtkWin32OffscreenRenderWindow_h

#include "vtkWin32OpenGLRenderWindow.h"

class VTK_EXPORT vtkWin32OffscreenRenderWindow : public vtkWin32OpenGLRenderWindow
{
public:
  static vtkWin32OffscreenRenderWindow *New();
  vtkTypeMacro(vtkWin32OffscreenRenderWindow,vtkWin32OpenGLRenderWindow);
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
  vtkWin32OffscreenRenderWindow();
  ~vtkWin32OffscreenRenderWindow();
  vtkWin32OffscreenRenderWindow(const vtkWin32OffscreenRenderWindow&) {};
  void operator=(const vtkWin32OffscreenRenderWindow&) {};

  HBITMAP MhBitmap, MhOldBitmap;
  int MBpp, MZBpp;
  
  // overrides
  virtual void Clean();
  virtual void WindowRemap(void) {} // not used
  virtual void PrefFullScreen(void) {} // not used
//ETX
};

#endif // __vtkWin32OffscreenRenderWindow_h
