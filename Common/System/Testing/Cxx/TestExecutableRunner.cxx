// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkExecutableRunner.h"
#include "vtkNew.h"

#include "vtkLogger.h"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

int TestExecutableRunner(int, char*[])
{
  vtkNew<vtkExecutableRunner> process;
  process->SetCommand("echo Hello World");
  process->Execute();
  std::string out = process->GetStdOut();
  std::string err = process->GetStdErr();
  int code = process->GetReturnValue();
  int returnValue = EXIT_SUCCESS;

  // ---
  if (code != 0)
  {
    std::cerr << " === ERROR: command did not succeed" << std::endl;
    returnValue = EXIT_FAILURE;
  }
  if (out != "Hello World")
  {
    std::cerr << " === ERROR: wrong command output. Got '" << out << "' but expected 'Hello World'."
              << std::endl;
    returnValue = EXIT_FAILURE;
  }
  if (!err.empty())
  {
    std::cerr << " === ERROR: there is output in the error stream : \n --- \n"
              << err << "\n --- " << std::endl;
    returnValue = EXIT_FAILURE;
  }

  // ---
  process->Execute();
  if (process->GetStdOut() != out || process->GetStdErr() != err ||
    process->GetReturnValue() != code)
  {
    std::cerr << " === ERROR: ran twice the same command, expected the same result" << std::endl;
    returnValue = EXIT_FAILURE;
  }

  // ---
  process->SetCommand("abcdefghijklmnopqrstuvw");
  // Disable global logger for this test as we don't want the error returned by the filter
  // to mess with our test
  int warning = vtkObject::GetGlobalWarningDisplay();
  vtkObject::SetGlobalWarningDisplay(0);
  process->Execute();
  vtkObject::SetGlobalWarningDisplay(warning);
  code = process->GetReturnValue();
  if (code == 0)
  {
    std::cerr << " === ERROR: command did not return a failure but was supposed to." << std::endl;
    returnValue = EXIT_FAILURE;
  }

  // ---
  process->SetExecuteInSystemShell(false);
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
  process->SetCommand("cmd.exe");
  process->AddArgument("/c");
  process->AddArgument("echo Hello World");
#else
  process->SetCommand("echo");
  process->AddArgument("Hello World");
#endif
  process->Execute();
  out = process->GetStdOut();
  err = process->GetStdErr();
  code = process->GetReturnValue();
  if (code != 0 || out != "Hello World" || !err.empty())
  {
    std::cerr << " === ERROR: wrong result with SetExecuteInSystemShell(false) option" << std::endl;
    returnValue = EXIT_FAILURE;
  }

  return returnValue;
}
