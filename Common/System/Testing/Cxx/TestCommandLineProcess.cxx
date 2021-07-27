#include "vtkCommandLineProcess.h"
#include "vtkNew.h"

#include "vtkLogger.h"

#include <iostream>
#include <string>

int TestCommandLineProcess(int, char*[])
{
  vtkNew<vtkCommandLineProcess> process;
  process->SetCommand("echo \"Hello World\"");
  process->Execute();
  std::string out = process->GetStdOut();
  std::string err = process->GetStdErr();
  int code = process->GetReturnValue();

  // ---
  if (code != 0)
  {
    std::cerr << "ERROR: command did not succeed" << std::endl;
    return EXIT_FAILURE;
  }
  if (out != "Hello World")
  {
    std::cerr << "ERROR: wrong command output" << std::endl;
    return EXIT_FAILURE;
  }
  if (!err.empty())
  {
    std::cerr << "ERROR: there is output in the error stream" << std::endl;
    return EXIT_FAILURE;
  }

  // ---
  process->Execute();
  if (process->GetStdOut() != out || !std::string(process->GetStdErr()).empty())
  {
    std::cerr << "ERROR: ran twice the same process, expected the same result" << std::endl;
    return EXIT_FAILURE;
  }

  // ---
  process->SetCommand("abcdefghijklmnopqrstuvw");
  // Disable gloabal logger for this test as we don't want the error returned by the filter
  // to mess with our test
  int warning = vtkObject::GetGlobalWarningDisplay();
  vtkObject::SetGlobalWarningDisplay(0);
  process->Execute();
  vtkObject::SetGlobalWarningDisplay(warning);
  code = process->GetReturnValue();
  if (code == 0)
  {
    std::cerr << "ERROR: command did not return a failure but was supposed to." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
