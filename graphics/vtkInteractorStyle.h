/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyle.h
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

// .NAME vtkInteractorStyle - provide event driven interface to rendering window

// .SECTION Description
// vtkInteractorStyle is a base class performing the majority of motion control
// routines and am event driven interface to RenderWindowInteractor which
// implements platform dependent key/m0ouse routing and timer control.
//
// vtkInteractorStyle can be subclassed to provide new interaction styles and
// a facility to override any of the default mouse/key operations which
// currently handle trackball or joystick styles is provided
//

#ifndef __vtkInteractorStyle_h
#define __vtkInteractorStyle_h

#include "vtkRenderWindowInteractor.h"

// motion flags
#define VTKIS_START   0
#define VTKIS_ROTATE 1
#define VTKIS_ZOOM   2
#define VTKIS_PAN    3
#define VTKIS_SPIN   4
#define VTKIS_DOLLY  5
#define VTKIS_USCALE 6
#define VTKIS_TIMER  7 
#define VTKIS_ANIM_OFF 0
#define VTKIS_ANIM_ON  1

class vtkPolyDataMapper;
class vtkOutlineSource;

class VTK_EXPORT vtkInteractorStyle : public vtkObject 
{
public:
  // Description:
  // This class must be supplied with a vtkRenderWindowInteractor wrapper or
  // parent. This class should not normally be instantiated by application
  // programmers.
  static vtkInteractorStyle *New();

  const char *GetClassName() {return "vtkInteractorStyle";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the Interactor wrapper being controlled by this object.
  void SetInteractor(vtkRenderWindowInteractor *interactor);
  vtkGetObjectMacro(Interactor, vtkRenderWindowInteractor);

  // Description:
  // When an event occurs, we must determine which Renderer the event
  // occurred within, since one RenderWindow may contain multiple
  // renderers. We also need to know what camera to operate on.
  // This is just the ActiveCamera of the poked renderer.
  void FindPokedCamera(int,int);
  void FindPokedRenderer(int,int);

  // Description:
  // When pick action successfully selects actor, this method highlights the
  // actor appropriately. Currently this is done by placing a bounding box
  // around the actor.
  virtual void HighlightActor(vtkActor *actor);

  // Description:
  // Generic event bindings must be overridden in subclasses
  virtual void OnMouseMove  (int ctrl, int shift, int X, int Y);
  virtual void OnLeftButtonDown(int ctrl, int shift, int X, int Y);
  virtual void OnLeftButtonUp  (int ctrl, int shift, int X, int Y);
  virtual void OnMiddleButtonDown(int ctrl, int shift, int X, int Y);
  virtual void OnMiddleButtonUp  (int ctrl, int shift, int X, int Y);
  virtual void OnRightButtonDown(int ctrl, int shift, int X, int Y);
  virtual void OnRightButtonUp  (int ctrl, int shift, int X, int Y);

  // Description:
  // OnChar implements keybaord functions, but subclasses can override this 
  // behaviour
  virtual void OnChar(int ctrl, int shift, char keycode, int repeatcount);

  // Description:
  // OnTimer calls RotateCamera, RotateActor etc which should be overridden by
  // style subclasses.
  virtual void OnTimer();

  // Description:
  // Callbacks so that the applicaiton can override the default behaviour.
  void SetLeftButtonPressMethod(void (*f)(void *), void *arg);
  void SetLeftButtonPressMethodArgDelete(void (*f)(void *));
  void SetLeftButtonReleaseMethod(void (*f)(void *), void *arg);
  void SetLeftButtonReleaseMethodArgDelete(void (*f)(void *));
  void SetMiddleButtonPressMethod(void (*f)(void *), void *arg);
  void SetMiddleButtonPressMethodArgDelete(void (*f)(void *));
  void SetMiddleButtonReleaseMethod(void (*f)(void *), void *arg);
  void SetMiddleButtonReleaseMethodArgDelete(void (*f)(void *));
  void SetRightButtonPressMethod(void (*f)(void *), void *arg);
  void SetRightButtonPressMethodArgDelete(void (*f)(void *));
  void SetRightButtonReleaseMethod(void (*f)(void *), void *arg);
  void SetRightButtonReleaseMethodArgDelete(void (*f)(void *));

protected:
  vtkInteractorStyle();
  ~vtkInteractorStyle();
  vtkInteractorStyle(const vtkInteractorStyle&) {};
  void operator=(const vtkInteractorStyle&) {};

  // convenience methods for converting between coordinate systems
  virtual void ComputeDisplayToWorld(double x, double y, double z,
                                     double *worldPt);
  virtual void ComputeWorldToDisplay(double x, double y, double z,
                                     double *displayPt);
  virtual void ComputeDisplayToWorld(double x, double y, double z,
                                     float *worldPt);
  virtual void ComputeWorldToDisplay(double x, double y, double z,
                                     float *displayPt);

  virtual void UpdateInternalState(int ctrl, int shift, int X, int Y);

  // These methods for the different interactions in different modes
  // are overridden in subclasses to perform the correct motion
  // This class provides a default implementation.

  virtual void RotateCamera(int x, int y);
  virtual void SpinCamera(int x, int y);
  virtual void PanCamera(int x, int y);
  virtual void DollyCamera(int x, int y);

  // utility routines used by state changes below
  virtual void StartState(int newstate);
  virtual void StopState();

  // Interaction mode entry points used internally.  
  virtual void StartAnimate();  
  virtual void StopAnimate();  
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
  virtual void StartTimer();
  virtual void EndTimer();

  //
  // Data we need to maintain internally
  vtkRenderWindowInteractor *Interactor;
  //
  vtkCamera          *CurrentCamera;
  vtkLight           *CurrentLight;
  vtkRenderer        *CurrentRenderer;
  //
  float Center[2];
  float DeltaAzimuth;
  float DeltaElevation;
  int   CtrlKey;
  int   ShiftKey;
  int   LastPos[2];
  int   State;  
  int   AnimState;  
  float FocalDepth;  

  // for picking actors
  vtkOutlineSource   *Outline;
  vtkPolyDataMapper  *OutlineMapper;
  vtkActor           *OutlineActor;
  vtkRenderer        *PickedRenderer;
  vtkActor           *CurrentActor;
  int                 ActorPicked;          // boolean: actor picked?

  void (*LeftButtonPressMethod)(void *);
  void (*LeftButtonPressMethodArgDelete)(void *);
  void *LeftButtonPressMethodArg;
  void (*LeftButtonReleaseMethod)(void *);
  void (*LeftButtonReleaseMethodArgDelete)(void *);
  void *LeftButtonReleaseMethodArg;

  void (*MiddleButtonPressMethod)(void *);
  void (*MiddleButtonPressMethodArgDelete)(void *);
  void *MiddleButtonPressMethodArg;
  void (*MiddleButtonReleaseMethod)(void *);
  void (*MiddleButtonReleaseMethodArgDelete)(void *);
  void *MiddleButtonReleaseMethodArg;

  void (*RightButtonPressMethod)(void *);
  void (*RightButtonPressMethodArgDelete)(void *);
  void *RightButtonPressMethodArg;
  void (*RightButtonReleaseMethod)(void *);
  void (*RightButtonReleaseMethodArgDelete)(void *);
  void *RightButtonReleaseMethodArg;
};

#endif
