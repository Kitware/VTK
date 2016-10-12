/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOutputWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOutputWindow
 * @brief   base class for writing debug output to a console
 *
 * This class is used to encapsulate all text output, so that it will work
 * with operating systems that have a stdout and stderr, and ones that
 * do not.  (i.e windows does not).  Sub-classes can be provided which can
 * redirect the output to a window.
*/

#ifndef vtkOutputWindow_h
#define vtkOutputWindow_h

#include "vtkDebugLeaksManager.h" // Must be included before singletons
#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

class VTKCOMMONCORE_EXPORT vtkOutputWindowCleanup
{
public:
  vtkOutputWindowCleanup();
  ~vtkOutputWindowCleanup();

private:
  vtkOutputWindowCleanup(const vtkOutputWindowCleanup& other) VTK_DELETE_FUNCTION;
  vtkOutputWindowCleanup& operator=(const vtkOutputWindowCleanup& rhs) VTK_DELETE_FUNCTION;
};

class VTKCOMMONCORE_EXPORT vtkOutputWindow : public vtkObject
{
public:
// Methods from vtkObject
  vtkTypeMacro(vtkOutputWindow,vtkObject);
  /**
   * Print ObjectFactor to stream.
   */
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * This is a singleton pattern New.  There will only be ONE
   * reference to a vtkOutputWindow object per process.  Clients that
   * call this must call Delete on the object so that the reference
   * counting will work.   The single instance will be unreferenced when
   * the program exits.
   */
  static vtkOutputWindow* New();
  /**
   * Return the singleton instance with no reference counting.
   */
  static vtkOutputWindow* GetInstance();
  /**
   * Supply a user defined output window. Call ->Delete() on the supplied
   * instance after setting it.
   */
  static void SetInstance(vtkOutputWindow *instance);
  //@{
  /**
   * Display the text. Four virtual methods exist, depending on the type of
   * message to display. This allows redirection or reformatting of the
   * messages. The default implementation uses DisplayText for all.
   */
  virtual void DisplayText(const char*);
  virtual void DisplayErrorText(const char*);
  virtual void DisplayWarningText(const char*);
  virtual void DisplayGenericWarningText(const char*);
  //@}

  virtual void DisplayDebugText(const char*);
  //@{
  /**
   * If PromptUser is set to true then each time a line of text
   * is displayed, the user is asked if they want to keep getting
   * messages.
   */
  vtkBooleanMacro(PromptUser,int);
  vtkSetMacro(PromptUser, int);
  //@}

protected:
  vtkOutputWindow();
  ~vtkOutputWindow() VTK_OVERRIDE;
  int PromptUser;
private:
  static vtkOutputWindow* Instance;
private:
  vtkOutputWindow(const vtkOutputWindow&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOutputWindow&) VTK_DELETE_FUNCTION;
};

// Uses schwartz counter idiom for singleton management
static vtkOutputWindowCleanup vtkOutputWindowCleanupInstance;


#endif
