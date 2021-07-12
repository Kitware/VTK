/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCommandLineProcess.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkCommandLineProcess_h
#define vtkCommandLineProcess_h

#include "vtkCommonSystemModule.h" // For export macro
#include "vtkObject.h"

#include "vtksys/Process.h" // For class vtksysProcess

#include <string> // fot class std::string

/**
 * @class   vtkCommandLineProcess
 * @brief   Launch a process on the current machine and get its output
 *
 * Launch a process on the current machine and get its standard output and
 * standard error output.
 */
class VTKCOMMONSYSTEM_EXPORT vtkCommandLineProcess : public vtkObject
{
public:
  static vtkCommandLineProcess* New();
  vtkTypeMacro(vtkCommandLineProcess, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkCommandLineProcess();
  virtual ~vtkCommandLineProcess();

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
  vtkGetStringMacro(Command);
  vtkSetStringMacro(Command);
  //@}

  //@{
  /**
   * Get output of the previously run command.
   */
  vtkGetStringMacro(StdOut);
  vtkGetStringMacro(StdErr);
  //@}

  /**
   * Get return value of last command. If no command has been
   * executed or if the command has failed in some way value is != 0,
   * else return 0.
   */
  vtkGetMacro(ReturnValue, int);

protected:
  vtkSetStringMacro(StdOut);
  vtkSetStringMacro(StdErr);

  int ExitProcess(vtksysProcess* process);

private:
  vtkCommandLineProcess(const vtkCommandLineProcess&) = delete;
  void operator=(const vtkCommandLineProcess&) = delete;

  bool RightTrimResult = true;
  double Timeout = 5;
  char* Command = nullptr;
  int ReturnValue = -1;

  char* StdOut = nullptr;
  char* StdErr = nullptr;
};

#endif // vtkCommandLineProcess_h
