/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXRenderWindowInteractor.h
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

#include "vtkRenderWindowInteractor.h"
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>

class VTK_EXPORT vtkXRenderWindowInteractor : public vtkRenderWindowInteractor
{
public:

// Description:
// Construct an instance so that the light follows the camera motion.
  vtkXRenderWindowInteractor();

  ~vtkXRenderWindowInteractor();
  static vtkXRenderWindowInteractor *New() {return new vtkXRenderWindowInteractor;};
  const char *GetClassName() {return "vtkXRenderWindowInteractor";};
  void PrintSelf(ostream& os, vtkIndent indent);


// Description:
// Initializes the event handlers without an XtAppContext.  This is
// good for when you don't have a user interface, but you still
// want to have mouse interaction.
  virtual void Initialize();


// Description: 
// Initializes the event handlers using an XtAppContext that you have
// provided.  This assumes that you want to own the event loop.
  virtual void Initialize(XtAppContext app);


// Description:
// This will start up the X event loop and never return. If you
// call this method it will loop processing X events until the
// application is exited.
  virtual void Start();

  virtual void UpdateSize(int,int);
  virtual void StartRotate();
  virtual void EndRotate();
  virtual void StartZoom();
  virtual void EndZoom();
  virtual void StartPan();
  virtual void EndPan();

// Description:
// Specify the Xt widget to use for interaction. This method is
// one of a couple steps that are required for setting up a
// vtkRenderWindowInteractor as a widget inside of another user 
// interface. You do not need to use this method if the render window
// will be a stand-alone window. This is only used when you want the
// render window to be a subwindow within a larger user interface.
// In that case, you must tell the render window what X display id
// to use, and then ask the render window what depth, visual and 
// colormap it wants. Then, you must create an Xt TopLevelShell with
// those settings. Then you can create the rest of your user interface
// as a child of the TopLevelShell you created. Eventually, you will 
// create a drawing area or some other widget to serve as the rendering
// window. You must use the SetWidget method to tell this Interactor
// about that widget. It's X and it's not terribly easy, but it looks cool.
  virtual void SetWidget(Widget);

  virtual void SetupNewWindow(int Stereo = 0);

// Description:
// Finish setting up a new window after the WindowRemap.
  virtual void FinishSettingUpNewWindow();

  
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
};

#endif


