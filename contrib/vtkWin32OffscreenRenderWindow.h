/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32OffscreenRenderWindow.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    to Horst Schreiber for developing this MFC code

Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/

#ifndef __vtkWin32OffscreenRenderWindow_h
#define __vtkWin32OffscreenRenderWindow_h

#include "vtkWin32OpenGLRenderWindow.h"

class VTK_EXPORT vtkWin32OffscreenRenderWindow : public vtkWin32OpenGLRenderWindow
{
public:
  static vtkWin32OffscreenRenderWindow *New();
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
