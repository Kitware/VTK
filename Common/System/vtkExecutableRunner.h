/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExecutableRunner.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkExecutableRunner_h
#define vtkExecutableRunner_h

#include "vtkCommonSystemModule.h" // For export macro
#include "vtkObject.h"

#include "vtksys/Process.h" // For class vtksysProcess

#include <string> // fot class std::string

/**
 * @class   vtkExecutableRunner
 * @brief   Launch a process on the current machine and get its output
 *
 * Launch a process on the current machine and get its standard output and
 * standard error output.
 */
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

  //@{
  /**
   * Set/Get command timeout in seconds.  A non-positive (<= 0) value will
   * disable the timeout.
   *
   * Default is 5
   */
  vtkSetMacro(Timeout, double);
  vtkGetMacro(Timeout, double);
  //@}

  //@{
  /**
   * Set/Get if we trim the ending whitespaces of the output.
   *
   * Default is true.
   */
  vtkSetMacro(RightTrimResult, bool);
  vtkGetMacro(RightTrimResult, bool);
  vtkBooleanMacro(RightTrimResult, bool);
  //@}

  //@{
  /**
   * Set/Get command to execute. An empty command will do nothing.
   */
  vtkGetCharFromStdStringMacro(Command);
  vtkSetStdStringFromCharMacro(Command);
  //@}

  //@{
  /**
   * Get output of the previously run command.
   */
  vtkGetCharFromStdStringMacro(StdOut);
  vtkGetCharFromStdStringMacro(StdErr);
  //@}

  /**
   * Get return value of last command. If no command has been
   * executed or if the command has failed in some way value is != 0,
   * else return 0.
   */
  vtkGetMacro(ReturnValue, int);

protected:
  vtkSetMacro(StdOut, std::string);
  vtkSetMacro(StdErr, std::string);

  int ExitProcess(vtksysProcess* process);

private:
  vtkExecutableRunner(const vtkExecutableRunner&) = delete;
  void operator=(const vtkExecutableRunner&) = delete;

  bool RightTrimResult = true;
  double Timeout = 5;
  std::string Command;
  int ReturnValue = -1;

  std::string StdOut;
  std::string StdErr;
};

#endif // vtkExecutableRunner_h
