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
// event bindings to common graphics functions. For example, camera and actor
// functions such as zoom-in/zoom-out, azimuth, roll, and pan. It is one of
// the window system specific subclasses of vtkRenderWindowInteractor.

// Mouse bindings:
//    camera: Button 1 - rotate; Button 2 - pan; and Button 3 - zoom;
//            ctrl-Button 1 - spin.
//    actor:  Button 1 - rotate; Button 2 - pan; Button 3 - uniform scale;
//            ctrl-Button 1 - spin; ctrl-Button 2 - dolly.
//
// Camera mode is the default mode for compatibility reasons
//
// When "j" is pressed, the interaction models after a joystick. The distance
// from the center of the renderer viewport determines how quickly to rotate,
// pan, zoom, spin, and dolly.  This is the default mode for compatiblity
// reasons.  This is also known as position sensitive motion.
//
// When "t" is pressed, the interaction models after a trackball. Each mouse
// movement is used to move the actor or camera. When the mouse stops, the
// camera or actor motion is also stopped. This is also known as motion
// sensitive motion.
//
// Rotate, pan, and zoom work the same way as before.  Spin has two different
// interfaces depending on whether the interactor is in trackball or joystick
// mode.  In trackball mode, by moving the mouse around the camera or actor
// center in a circular motion, the camera or actor is spun.  In joystick mode
// by moving the mouse in the y direction, the actor or camera is spun. Scale
// dolly, and zoom all work in the same manner, that motion of mouse in y
// direction generates the transformation.
//
// There are no difference between camera and actor mode interactions, which
// means that the same events elicit the same responses
//
// Actor picking can be accomplished with the "p" key or with a mouse click
// in actor mode. 
//
// Keystrokes:
//    j - joystick-like mouse interactions
//    t - trackball-like mouse interactions
//    o - object / actor interaction
//    c - camera interaction
//    r - reset camera view
//    w - turn all actors wireframe
//    s - turn all actors surface
//    u - execute user defined function
//    p - pick actor under mouse pointer (if pickable)
//    3 - toggle in/out of 3D mode (if supported by renderer)
//    e - exits
//    q - exits

// .SECTION see also
// vtkRenderWindowInteractor vtkWin32OpenGLRenderWindow



#ifndef __vtkWin32RenderWindowInteractor_h
#define __vtkWin32RenderWindowInteractor_h

#include <stdlib.h>
#include "vtkRenderWindowInteractor.h"

class VTK_EXPORT vtkWin32RenderWindowInteractor : public vtkRenderWindowInteractor
{
public:

  // Description:
  // Construct object so that light follows camera motion.
  vtkWin32RenderWindowInteractor();
  ~vtkWin32RenderWindowInteractor();
  static vtkWin32RenderWindowInteractor *New() {
    return new vtkWin32RenderWindowInteractor;};
  const char *GetClassName() {return "vtkWin32RenderWindowInteractor";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialize the even handler
  virtual void Initialize();

  // Description:
  // This will start up the event loop and never return. If you
  // call this method it will loop processing events until the
  // application is exited.
  virtual void Start();
  void UpdateSize(int,int);

  // Description:
  // Provide implementaitons of the methods defined in
  // vtkRenderWindowInteractor. Generally the application developer should
  // not invoke these methods directly.
  virtual void StartRotate();
  virtual void EndRotate();
  virtual void StartZoom();
  virtual void EndZoom();
  virtual void StartPan();
  virtual void EndPan();
  virtual void StartSpin();
  virtual void EndSpin();
  virtual void StartDolly();
  virtual void EndDolly();
  virtual void StartUniformScale();
  virtual void EndUniformScale();

  virtual void StartAnimation();
  virtual void EndAnimation();

  //BTX
  friend LRESULT CALLBACK vtkHandleMessage(HWND hwnd,UINT uMsg,
					   WPARAM w, LPARAM l);
  //ETX

  // Description:
  // Methods to set the default exit method for the class. This method is
  // only used if no instance level ExitMethod has been defined.  It is
  // provided as a means to control how an interactor is exited given
  // the various language bindings (tcl, Win32, etc.).
  static void SetClassExitMethod(void (*f)(void *), void *arg);
  static void SetClassExitMethodArgDelete(void (*f)(void *));
  
protected:
  HWND WindowId;
  UINT TimerId;
  WNDPROC OldProc;
  LPARAM LastPosition;
  
  //BTX
  // Description:
  // Class variables so an exit method can be defined for this class
  // (used to set different exit methods for various language bindings,
  // i.e. tcl, java, Win32)
  static void (*ClassExitMethod)(void *);
  static void (*ClassExitMethodArgDelete)(void *);
  static void *ClassExitMethodArg;
  //ETX
};

#endif


