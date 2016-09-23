/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIOSRenderWindowInteractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkIOSRenderWindowInteractor
 * @brief   implements IOS specific functions
 * required by vtkRenderWindowInteractor.
 *
 *
 * The interactor interfaces with vtkIOSRenderWindow and vtkIOSGLView
 * to trap messages from the IOS window manager and send them to vtk.
 *
 * IMPORTANT: This header must be in C++ only because it is included by .cxx files.
 * That means no Objective C may be used. That's why some instance variables are
 * void* instead of what they really should be.
*/

#ifndef vtkIOSRenderWindowInteractor_h
#define vtkIOSRenderWindowInteractor_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkRenderWindowInteractor.h"

class VTKRENDERINGOPENGL2_EXPORT vtkIOSRenderWindowInteractor : public vtkRenderWindowInteractor
{
public:
  /**
   * Construct object so that light follows camera motion.
   */
  static vtkIOSRenderWindowInteractor *New();

  vtkTypeMacro(vtkIOSRenderWindowInteractor,vtkRenderWindowInteractor);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Initialize the even handler
   */
  virtual void Initialize();

  //@{
  /**
   * Enable/Disable interactions.  By default interactors are enabled when
   * initialized.  Initialize() must be called prior to enabling/disabling
   * interaction. These methods are used when a window/widget is being
   * shared by multiple renderers and interactors.  This allows a "modal"
   * display where one interactor is active when its data is to be displayed
   * and all other interactors associated with the widget are disabled
   * when their data is not displayed.
   */
  virtual void Enable();
  virtual void Disable();
  //@}

  /**
   * IOS specific application terminate, calls ClassExitMethod then
   * calls PostQuitMessage(0) to terminate app. An application can Specify
   * ExitMethod for alternative behaviour (i.e. suppresion of keyboard exit)
   */
  void TerminateApp();

  //@{
  /**
   * Methods to set the default exit method for the class. This method is
   * only used if no instance level ExitMethod has been defined.  It is
   * provided as a means to control how an interactor is exited given
   * the various language bindings (tcl, IOS, etc.).
   */
  static void SetClassExitMethod(void (*f)(void *), void *arg);
  static void SetClassExitMethodArgDelete(void (*f)(void *));
  //@}

  /**
   * These methods correspond to the the Exit, User and Pick
   * callbacks. They allow for the Style to invoke them.
   */
  virtual void ExitCallback();

//  int GetButtonDown();
//  void SetButtonDown(int button);

protected:
  vtkIOSRenderWindowInteractor();
  ~vtkIOSRenderWindowInteractor();

  /**
   * Accessors for the IOS member variables. These should be used at all time, even
   * by this class.
   */
  void SetTimerDictionary(void *dictionary);    // Really an NSMutableDictionary*
  void *GetTimerDictionary();

  //@{
  /**
   * Class variables so an exit method can be defined for this class
   * (used to set different exit methods for various language bindings,
   * i.e. tcl, java, IOS)
   */
  static void (*ClassExitMethod)(void *);
  static void (*ClassExitMethodArgDelete)(void *);
  static void *ClassExitMethodArg;
  //@}

  //@{
  /**
   * IOS-specific internal timer methods. See the superclass for detailed
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

  //@{
  /**
   * Accessors for the IOS manager (Really an NSMutableDictionary*).
   * It manages all IOS objects in this C++ class.
   */
  void SetIOSManager(void *manager);
  void *GetIOSManager();
  //@}

private:
  vtkIOSRenderWindowInteractor(const vtkIOSRenderWindowInteractor&) VTK_DELETE_FUNCTION;
  void operator=(const vtkIOSRenderWindowInteractor&) VTK_DELETE_FUNCTION;

  // Important: this class cannot contain Objective-C instance
  // variables for 2 reasons:
  // 1) C++ files include this header
  // 2) because of garbage collection
  // Instead, use the IOSManager dictionary to keep a collection
  // of what would otherwise be Objective-C instance variables.
  void    *IOSManager;        // Really an NSMutableDictionary*

};

#endif
