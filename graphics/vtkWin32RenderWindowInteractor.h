/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32RenderWindowInteractor.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
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
// .NAME vtkWin32RenderWindowInteractor - provide an event driven interface 
// to the renderer
// .SECTION Description
// vtkWin32RenderWindowInteractor is a convenience object that provides event 
// event bindings to common graphics functions. For example, camera
// zoom-in/zoom-out, azimuth, and roll. It is one of the window system
// specific subclasses of vtkRenderWindowInteractor.

// .SECTION see also
// vtkRenderWindowInteractor vtkWin32OpenGLRenderWindow

// .SECTION Event Bindings
// Mouse bindings: Button 1 - rotate, Button 2 - pan, Button 3 - zoom
// The distance from the center of the renderer viewport determines
// how quickly to rotate, pan and zoom.
// Keystrokes:
//    r - reset camera view
//    w - turn all actors wireframe
//    s - turn all actors surface
//    e - exits


#ifndef __vtkWin32RenderWindowInteractor_h
#define __vtkWin32RenderWindowInteractor_h

#include "vtkRenderWindowInteractor.h"

class VTK_EXPORT vtkWin32RenderWindowInteractor : public vtkRenderWindowInteractor
{
public:
  vtkWin32RenderWindowInteractor();
  ~vtkWin32RenderWindowInteractor();
  static vtkWin32RenderWindowInteractor *New() {return new vtkWin32RenderWindowInteractor;};
  const char *GetClassName() {return "vtkWin32RenderWindowInteractor";};
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Initialize();
  virtual void Start();
  void UpdateSize(int,int);
  void StartRotate();
  void EndRotate();
  void StartZoom();
  void EndZoom();
  void StartPan();
  void EndPan();
  //BTX
  friend LRESULT CALLBACK vtkHandleMessage(HWND hwnd,UINT uMsg,
					   WPARAM w, LPARAM l);
  //ETX
protected:
  HWND WindowId;
  UINT TimerId;
  WNDPROC OldProc;
};

#endif


