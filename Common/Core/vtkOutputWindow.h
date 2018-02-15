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
  vtkOutputWindowCleanup(const vtkOutputWindowCleanup& other) = delete;
  vtkOutputWindowCleanup& operator=(const vtkOutputWindowCleanup& rhs) = delete;
};

class VTKCOMMONCORE_EXPORT vtkOutputWindow : public vtkObject
{
public:
// Methods from vtkObject
  vtkTypeMacro(vtkOutputWindow,vtkObject);
  /**
   * Print ObjectFactor to stream.
   */
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates a new instance of vtkOutputWindow. Note this *will* create a new
   * instance using the vtkObjectFactor. If you want to access the global
   * instance, use `GetInstance` instead.
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
   * Consequently, subclasses can simply override DisplayText and use
   * `GetCurrentMessageType` to determine the type of message that's being reported.
   */
  virtual void DisplayText(const char*);
  virtual void DisplayErrorText(const char*);
  virtual void DisplayWarningText(const char*);
  virtual void DisplayGenericWarningText(const char*);
  virtual void DisplayDebugText(const char*);
  //@}

  //@{
  /**
   * If PromptUser is set to true then each time a line of text
   * is displayed, the user is asked if they want to keep getting
   * messages.
   *
   * Note that PromptUser has not effect of messages displayed by directly
   * calling `DisplayText`. The prompt is never shown for such messages.
   *
   */
  vtkBooleanMacro(PromptUser, bool);
  vtkSetMacro(PromptUser, bool);
  //@}

  //@{
  /**
   * Historically (VTK 8.1 and earlier), when printing messages to terminals,
   * vtkOutputWindow would always post messages to `cerr`. Setting this to true
   * restores that incorrect behavior. When false (default),
   * vtkOutputWindow uses `cerr` for debug, error and warning messages, and
   * `cout` for text messages.
   */
  vtkSetMacro(UseStdErrorForAllMessages, bool);
  vtkGetMacro(UseStdErrorForAllMessages, bool);
  vtkBooleanMacro(UseStdErrorForAllMessages, bool);
  //@}

protected:
  vtkOutputWindow();
  ~vtkOutputWindow() override;

  enum MessageTypes
  {
    MESSAGE_TYPE_TEXT,
    MESSAGE_TYPE_ERROR,
    MESSAGE_TYPE_WARNING,
    MESSAGE_TYPE_GENERIC_WARNING,
    MESSAGE_TYPE_DEBUG
  };

  /**
   * Returns the current message type. Useful in subclasses that simply want to
   * override `DisplayText` and also know what type of message is being
   * processed.
   */
  vtkGetMacro(CurrentMessageType, MessageTypes);

  bool PromptUser;
  bool UseStdErrorForAllMessages;

private:
  static vtkOutputWindow* Instance;
  MessageTypes CurrentMessageType;

private:
  vtkOutputWindow(const vtkOutputWindow&) = delete;
  void operator=(const vtkOutputWindow&) = delete;
};

// Uses schwartz counter idiom for singleton management
static vtkOutputWindowCleanup vtkOutputWindowCleanupInstance;


#endif
