/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCocoaRenderWindowInteractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCocoaRenderWindowInteractor
 * @brief   implements Cocoa specific functions
 * required by vtkRenderWindowInteractor.
 *
 *
 * The interactor interfaces with vtkCocoaRenderWindow and vtkCocoaGLView
 * to trap messages from the Cocoa window manager and send them to vtk.
 * Since OS X applications typically use the Command key where UNIX and
 * Windows applications would use the Ctrl key, this interactor maps the
 * Command key to Ctrl.  In versions of VTK prior to VTK 6.2, it was
 * mapped to Alt.  On OS X, the Option key can be used as Alt.
 *
 * IMPORTANT: This header must be in C++ only because it is included by .cxx
 * files.  That means no Objective C may be used. That's why some instance
 * variables are void* instead of what they really should be.
*/

#ifndef vtkCocoaRenderWindowInteractor_h
#define vtkCocoaRenderWindowInteractor_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkRenderWindowInteractor.h"
#include "vtkTDxConfigure.h" // defines VTK_USE_TDX
#ifdef VTK_USE_TDX
class vtkTDxMacDevice;
#endif

class VTKRENDERINGOPENGL_EXPORT vtkCocoaRenderWindowInteractor : public vtkRenderWindowInteractor
{
public:
  /**
   * Construct object so that light follows camera motion.
   */
  static vtkCocoaRenderWindowInteractor *New();

  vtkTypeMacro(vtkCocoaRenderWindowInteractor,vtkRenderWindowInteractor);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Initialize the even handler
   */
  void Initialize() VTK_OVERRIDE;

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
  void Enable() VTK_OVERRIDE;
  void Disable() VTK_OVERRIDE;
  //@}

  /**
   * Cocoa specific application terminate, calls ClassExitMethod then
   * calls PostQuitMessage(0) to terminate app. An application can Specify
   * ExitMethod for alternative behaviour (i.e. suppresion of keyboard exit)
   */
  void TerminateApp() VTK_OVERRIDE;

  //@{
  /**
   * Methods to set the default exit method for the class. This method is
   * only used if no instance level ExitMethod has been defined.  It is
   * provided as a means to control how an interactor is exited given
   * the various language bindings (tcl, Cocoa, etc.).
   */
  static void SetClassExitMethod(void (*f)(void *), void *arg);
  static void SetClassExitMethodArgDelete(void (*f)(void *));
  //@}

  /**
   * These methods correspond to the the Exit, User and Pick
   * callbacks. They allow for the Style to invoke them.
   */
  void ExitCallback() VTK_OVERRIDE;

//  int GetButtonDown();
//  void SetButtonDown(int button);

protected:
  vtkCocoaRenderWindowInteractor();
  ~vtkCocoaRenderWindowInteractor() VTK_OVERRIDE;

  /**
   * Accessors for the Cocoa member variables. These should be used at all time, even
   * by this class.
   */
  void SetTimerDictionary(void *dictionary);    // Really an NSMutableDictionary*
  void *GetTimerDictionary();

  //@{
  /**
   * Class variables so an exit method can be defined for this class
   * (used to set different exit methods for various language bindings,
   * i.e. tcl, java, Cocoa)
   */
  static void (*ClassExitMethod)(void *);
  static void (*ClassExitMethodArgDelete)(void *);
  static void *ClassExitMethodArg;
  //@}

  //@{
  /**
   * Cocoa-specific internal timer methods. See the superclass for detailed
   * documentation.
   */
  int InternalCreateTimer(int timerId, int timerType, unsigned long duration) VTK_OVERRIDE;
  int InternalDestroyTimer(int platformTimerId) VTK_OVERRIDE;
  //@}

  /**
   * This will start up the event loop and never return. If you
   * call this method it will loop processing events until the
   * application is exited.
   */
  void StartEventLoop() VTK_OVERRIDE;

  //@{
  /**
   * Accessors for the cocoa manager (Really an NSMutableDictionary*).
   * It manages all Cocoa objects in this C++ class.
   */
  void SetCocoaManager(void *manager);
  void *GetCocoaManager();
  //@}

#ifdef VTK_USE_TDX
  vtkTDxMacDevice *Device;
#endif

private:
  vtkCocoaRenderWindowInteractor(const vtkCocoaRenderWindowInteractor&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCocoaRenderWindowInteractor&) VTK_DELETE_FUNCTION;

  // Important: this class cannot contain Objective-C instance
  // variables for 2 reasons:
  // 1) C++ files include this header
  // 2) because of garbage collection
  // Instead, use the CocoaManager dictionary to keep a collection
  // of what would otherwise be Objective-C instance variables.
  void    *CocoaManager;        // Really an NSMutableDictionary*

};

#endif
