// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkExecutableRunner_h
#define vtkExecutableRunner_h

#include "vtkCommonSystemModule.h" // For export macro
#include "vtkObject.h"

#include "vtksys/Process.h" // For class vtksysProcess

#include <string> // for class std::string
#include <vector> // for class std::vector

/**
 * @class   vtkExecutableRunner
 * @brief   Launch a process on the current machine and get its output
 *
 * Launch a process on the current machine and get its standard output and
 * standard error output. When `ExecuteInSystemShell` is false, arguments
 * needs to be added separately using the `AddArgument` / `ClearArguments`
 * API, otherwise command may not work correctly. If one does not know how to
 * parse the arguments of the command it want to execute then
 * `ExecuteInSystemShell` should be set to true.
 */
VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONSYSTEM_EXPORT vtkExecutableRunner : public vtkObject
{
public:
  static vtkExecutableRunner* New();
  vtkTypeMacro(vtkExecutableRunner, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkExecutableRunner() = default;
  ~vtkExecutableRunner() override = default;

  /**
   * Execute the command currently set if any.
   * This will update the StdOut and StdErr properties.
   */
  void Execute();

  ///@{
  /**
   * Set/Get command timeout in seconds.  A non-positive (<= 0) value will
   * disable the timeout.
   *
   * Default is 5
   */
  vtkSetMacro(Timeout, double);
  vtkGetMacro(Timeout, double);
  ///@}

  ///@{
  /**
   * Set/Get if we trim the ending whitespaces of the output.
   *
   * Default is true.
   */
  vtkSetMacro(RightTrimResult, bool);
  vtkGetMacro(RightTrimResult, bool);
  vtkBooleanMacro(RightTrimResult, bool);
  ///@}

  ///@{
  /**
   * Set/Get command to execute. An empty command will do nothing.
   */
  vtkGetCharFromStdStringMacro(Command);
  vtkSetStdStringFromCharMacro(Command);
  ///@}

  ///@{
  /**
   * Allows the command to be launched using the system shell (`sh` on unix
   * systems, cmd.exe on windows). This is handy when the user doesn't know
   * how to split arguments from a single string.
   *
   * Default to true.
   */
  vtkSetMacro(ExecuteInSystemShell, bool);
  vtkGetMacro(ExecuteInSystemShell, bool);
  vtkBooleanMacro(ExecuteInSystemShell, bool);
  ///@}

  ///@{
  /**
   * API to control arguments passed to the command when `ExecuteInSystemShell`
   * is false.
   *
   * Default is no argument.
   */
  virtual void AddArgument(const std::string& arg);
  virtual void ClearArguments();
  virtual vtkIdType GetNumberOfArguments() const;
  ///@}

  ///@{
  /**
   * Get output of the previously run command.
   */
  vtkGetCharFromStdStringMacro(StdOut);
  vtkGetCharFromStdStringMacro(StdErr);
  ///@}

  /**
   * Get return value of last command. If no command has been
   * executed or if the command has failed in some way value is != 0,
   * else return 0.
   */
  vtkGetMacro(ReturnValue, int);

protected:
  vtkSetMacro(StdOut, std::string);
  vtkSetMacro(StdErr, std::string);

  std::vector<std::string> GetCommandToExecute() const;
  int ExitProcess(vtksysProcess* process);

private:
  vtkExecutableRunner(const vtkExecutableRunner&) = delete;
  void operator=(const vtkExecutableRunner&) = delete;

  bool RightTrimResult = true;
  double Timeout = 5;
  std::string Command;
  int ReturnValue = -1;
  bool ExecuteInSystemShell = true;
  std::vector<std::string> Arguments;

  std::string StdOut;
  std::string StdErr;
};

VTK_ABI_NAMESPACE_END
#endif // vtkExecutableRunner_h
