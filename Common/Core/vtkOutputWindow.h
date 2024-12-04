// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkCommonCoreModule.h"  // For export macro
#include "vtkDebugLeaksManager.h" // Must be included before singletons
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkOutputWindowPrivateAccessor;
class VTKCOMMONCORE_EXPORT vtkOutputWindow : public vtkObject
{
public:
  // Methods from vtkObject
  vtkTypeMacro(vtkOutputWindow, vtkObject);
  /**
   * Print ObjectFactor to stream.
   */
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates a new instance of vtkOutputWindow. Note this *will* create a new
   * instance using the vtkObjectFactory. If you want to access the global
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
  static void SetInstance(vtkOutputWindow* instance);

  ///@{
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
  ///@}

  ///@{
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
  ///@}

  ///@{
  /**
   * Flag indicates how the vtkOutputWindow handles displaying of text to
   * `stderr` / `stdout`. Default is `DEFAULT` except in
   * `vtkWin32OutputWindow` where on non dashboard runs, the default is
   * `NEVER`.
   *
   * `NEVER` indicates that the messages should never be forwarded to the
   * standard output/error streams.
   *
   * `ALWAYS` will result in error/warning/debug messages being posted to the
   * standard error stream, while text messages to standard output stream.
   *
   * `ALWAYS_STDERR` will result in all messages being posted to the standard
   * error stream (this was default behavior in VTK 8.1 and earlier).
   *
   * `DEFAULT` is similar to `ALWAYS` except when logging is enabled. If
   * logging is enabled, messages posted to the output window using VTK error/warning macros such as
   * `vtkErrorMacro`, `vtkWarningMacro` etc. will not posted on any of the output streams. This is
   * done to avoid duplicate messages on these streams since these macros also result in add items
   * to the log.
   *
   * @note vtkStringOutputWindow does not result this flag as is never forwards
   * any text to the output streams.
   */
  enum DisplayModes
  {
    DEFAULT = -1,
    NEVER = 0,
    ALWAYS = 1,
    ALWAYS_STDERR = 2
  };
  vtkSetClampMacro(DisplayMode, int, DEFAULT, ALWAYS_STDERR);
  vtkGetMacro(DisplayMode, int);
  void SetDisplayModeToDefault() { this->SetDisplayMode(vtkOutputWindow::DEFAULT); }
  void SetDisplayModeToNever() { this->SetDisplayMode(vtkOutputWindow::NEVER); }
  void SetDisplayModeToAlways() { this->SetDisplayMode(vtkOutputWindow::ALWAYS); }
  void SetDisplayModeToAlwaysStdErr() { this->SetDisplayMode(vtkOutputWindow::ALWAYS_STDERR); }
  ///@}
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

  enum class StreamType
  {
    Null,
    StdOutput,
    StdError,
  };

  /**
   * Returns the standard output stream to post the message of the given type
   * on.
   */
  virtual StreamType GetDisplayStream(MessageTypes msgType) const;

  bool PromptUser;

private:
  std::atomic<MessageTypes> CurrentMessageType;
  int DisplayMode;
  std::atomic<int> InStandardMacros; // used to suppress display to output streams from standard
                                     // macros when logging is enabled.

  friend class vtkOutputWindowPrivateAccessor;

  vtkOutputWindow(const vtkOutputWindow&) = delete;
  void operator=(const vtkOutputWindow&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
