/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXRenderWindowInteractor.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkXRenderWindowInteractor - an X event driven interface for a RenderWindow
// .SECTION Description
// vtkXRenderWindowInteractor is a convenience object that provides
// event bindings to common graphics functions. For example, camera
// zoom-in/zoom-out, azimuth, and roll. It is one of the window system
// specific subclasses of vtkRenderWindowInteractor.

// .SECTION see also
// vtkRenderWindowInteractor vtkXRenderWindow

// .SECTION Event Bindings
// Mouse bindings: Button 1 - rotate; Button 2 - pan; and Button 3 - zoom.
// The distance from the center of the renderer viewport determines
// how quickly to rotate, pan and zoom.
// Keystrokes:
//    r - reset camera view
//    w - turn all actors wireframe
//    s - turn all actors surface
//    u - execute user defined function
//    p - pick actor under mouse pointer (if pickable)
//    3 - toggle in/out of 3D mode (if supported by renderer)
//    e - exit

#ifndef __vtkXRenderWindowInteractor_h
#define __vtkXRenderWindowInteractor_h

//===========================================================
// now we define the C++ class

#include "vtkRenderWindowInteractor.hh"
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>

class vtkXRenderWindowInteractor : public vtkRenderWindowInteractor
{
public:
  vtkXRenderWindowInteractor();
  ~vtkXRenderWindowInteractor();
  char *GetClassName() {return "vtkXRenderWindowInteractor";};
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Initialize();
  virtual void Initialize(XtAppContext app);
  virtual void Start();
  void UpdateSize(int,int);
  void StartRotate();
  void EndRotate();
  void StartZoom();
  void EndZoom();
  void StartPan();
  void EndPan();
  void SetWidget(Widget);
  void SetupNewWindow(int Stereo = 0);
  void FinishSettingUpNewWindow();
  
  friend void vtkXRenderWindowInteractorCallback(Widget,XtPointer,
					     XEvent *,Boolean *);
  friend void vtkXRenderWindowInteractorTimer(XtPointer,XtIntervalId *);

protected:
  Display *DisplayId;
  Window WindowId;
  Widget top;
  Widget oldTop;
  XtAppContext App;
  int PositionBeforeStereo[2];
  int WaitingForMarker;
};

#endif


