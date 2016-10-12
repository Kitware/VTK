/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOculusRenderWindowInteractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOculusRenderWindowInteractor
 * @brief   implements Oculus specific functions
 * required by vtkRenderWindowInteractor.
 *
 *
*/

#ifndef vtkOculusRenderWindowInteractor_h
#define vtkOculusRenderWindowInteractor_h

#include "vtkRenderingOculusModule.h" // For export macro
#include "vtkRenderWindowInteractor3D.h"

#include "vtkOculusRenderWindow.h" // ivars
#include "vtkNew.h" // ivars
#include "vtkTransform.h" // ivars

class VTKRENDERINGOCULUS_EXPORT vtkOculusRenderWindowInteractor : public vtkRenderWindowInteractor3D
{
public:
  /**
   * Construct object so that light follows camera motion.
   */
  static vtkOculusRenderWindowInteractor *New();

  vtkTypeMacro(vtkOculusRenderWindowInteractor,vtkRenderWindowInteractor3D);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Initialize the event handler
   */
  virtual void Initialize();

  /**
   * Oculus specific application terminate, calls ClassExitMethod then
   * calls PostQuitMessage(0) to terminate the application. An application can Specify
   * ExitMethod for alternative behavior (i.e. suppression of keyboard exit)
   */
  void TerminateApp(void);

  //@{
  /**
   * Methods to set the default exit method for the class. This method is
   * only used if no instance level ExitMethod has been defined.  It is
   * provided as a means to control how an interactor is exited given
   * the various language bindings (tcl, Win32, etc.).
   */
  static void SetClassExitMethod(void (*f)(void *), void *arg);
  static void SetClassExitMethodArgDelete(void (*f)(void *));
  //@}

  /**
   * These methods correspond to the the Exit, User and Pick
   * callbacks. They allow for the Style to invoke them.
   */
  virtual void ExitCallback();

  //@{
  /**
   * Set/Get the optional translation to map world coordinates into the
   * 3D physical space (meters, 0,0,0).
   */
  virtual void SetPhysicalTranslation(vtkCamera *, double, double, double);
  virtual double *GetPhysicalTranslation(vtkCamera *);
  //@}

protected:
  vtkOculusRenderWindowInteractor();
  ~vtkOculusRenderWindowInteractor();

  //@{
  /**
   * Class variables so an exit method can be defined for this class
   * (used to set different exit methods for various language bindings,
   * i.e. tcl, java, Win32)
   */
  static void (*ClassExitMethod)(void *);
  static void (*ClassExitMethodArgDelete)(void *);
  static void *ClassExitMethodArg;
  //@}

  //@{
  /**
   * Win32-specific internal timer methods. See the superclass for detailed
   * documentation.
   */
  virtual int InternalCreateTimer(int timerId, int timerType, unsigned long duration);
  virtual int InternalDestroyTimer(int platformTimerId);
  //@}

  /**
   * This will start up the event loop and never return. If you
   * call this method it will loop processing events until the
   * application is exited.
   */
  virtual void StartEventLoop();

  vtkNew<vtkTransform> PoseTransform;

private:
  vtkOculusRenderWindowInteractor(const vtkOculusRenderWindowInteractor&);  // Not implemented.
  void operator=(const vtkOculusRenderWindowInteractor&);  // Not implemented.
};

#endif
