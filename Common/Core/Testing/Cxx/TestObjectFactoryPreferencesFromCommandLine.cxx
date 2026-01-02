// SPDX-File-CopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCommand.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"

#include <sstream>
#include <unordered_set>
namespace
{
struct ScopedTestContext
{
  int OriginalArgC;
  int ArgC;
  char** ArgV;
  std::unordered_set<std::string> ReceivedWarnings;
  unsigned long WarningObserverTag;
  explicit ScopedTestContext(std::vector<std::string> args)
  {
    ArgC = 0;
    ArgV = new char*[args.size() + 1];
    std::ostringstream argStream;
    vtkLog(INFO, "Setting up simulated command line arguments " << args.size());
    for (const auto& arg : args)
    {
      vtkLog(INFO, "Adding argument: " << arg);
      argStream << arg << " ";
      ArgV[ArgC++] = strcpy(new char[arg.length() + 1], arg.c_str());
    }
    vtkLog(INFO, "Simulated command line arguments: " << argStream.str());
    ArgV[ArgC] = nullptr;
    OriginalArgC = ArgC;
    vtkOutputWindow* outputWindow = vtkOutputWindow::GetInstance();
    this->WarningObserverTag = outputWindow->AddObserver(
      vtkCommand::WarningEvent, this, &ScopedTestContext::WarningCallback);
  }

  ~ScopedTestContext()
  {
    // remove observer
    vtkOutputWindow* outputWindow = vtkOutputWindow::GetInstance();
    outputWindow->RemoveObserver(this->WarningObserverTag);
    // cleanup args
    for (int i = 0; i < ArgC; ++i)
    {
      delete[] ArgV[i];
    }
    for (int i = ArgC + 1; i < OriginalArgC; ++i)
    {
      delete[] ArgV[i];
    }
    delete[] ArgV;
  }

  void WarningCallback(vtkObject* vtkNotUsed(caller), unsigned long event, void* callData)
  {
    if (event == vtkCommand::WarningEvent)
    {
      std::string warningMessage = static_cast<const char*>(callData);
      this->ReceivedWarnings.insert(warningMessage);
    }
  }

  bool WarningMatches(const std::string& message) const
  {
    for (const auto& warning : this->ReceivedWarnings)
    {
      if (warning.find(message) != std::string::npos)
      {
        return true;
      }
    }
    return false;
  }

  bool HasWarnings() const { return !this->ReceivedWarnings.empty(); }
};
}

int TestObjectFactoryPreferencesFromCommandLine(int, char*[])
{
  bool success = true;
  {
    vtkLogScopeF(INFO, "Case 0");
    ScopedTestContext scopedTestCtx{ { "ExeName", "--another-arg", "--some-other-arg" } };
    if (vtkObjectFactory::InitializePreferencesFromCommandLineArgs(
          scopedTestCtx.ArgC, scopedTestCtx.ArgV))
    {
      vtkLog(ERROR,
        "Expected vtkObjectFactory::InitializePreferencesFromCommandLineArgs to return false");
      success = false;
    }
    if (scopedTestCtx.HasWarnings())
    {
      vtkLog(ERROR, "Unexpected warnings received");
      success = false;
    }
    // verify argument count remains the same
    if (scopedTestCtx.ArgC != scopedTestCtx.OriginalArgC)
    {
      vtkLog(ERROR, "Expected 3 arguments remaining, got " << scopedTestCtx.ArgC);
      success = false;
    }
  }
  {
    vtkLogScopeF(INFO, "Case 1");
    ScopedTestContext scopedTestCtx{ { "ExeName", "--another-arg", "--vtk-factory-prefer",
      "Type=Special;Level=2;Type=AnotherSpecial;Level=1", "--some-other-arg" } };
    if (!vtkObjectFactory::InitializePreferencesFromCommandLineArgs(
          scopedTestCtx.ArgC, scopedTestCtx.ArgV))
    {
      vtkLog(ERROR, "vtkObjectFactory::InitializePreferencesFromCommandLineArgs returned false");
      success = false;
    }
    if (scopedTestCtx.HasWarnings())
    {
      vtkLog(ERROR, "Unexpected warnings received");
      success = false;
    }
    // verify argument count was modified to remove processed ones
    if (scopedTestCtx.ArgC != scopedTestCtx.OriginalArgC - 2)
    {
      vtkLog(ERROR, "Expected 3 arguments remaining, got " << scopedTestCtx.ArgC);
      success = false;
    }
    for (int i = 0; i <= scopedTestCtx.OriginalArgC; ++i)
    {
      vtkLog(INFO,
        " Remaining Arg[" << i << "]='"
                          << (scopedTestCtx.ArgV[i] ? scopedTestCtx.ArgV[i] : "nullptr") << "'");
    }
    // the unprocessed arguments must take the place of the consumed ones.
    if (strcmp(scopedTestCtx.ArgV[0], "ExeName") > 0 ||
      strcmp(scopedTestCtx.ArgV[1], "--another-arg") > 0 ||
      strcmp(scopedTestCtx.ArgV[2], "--some-other-arg") > 0 || (scopedTestCtx.ArgV[3] != nullptr) ||
      strcmp(scopedTestCtx.ArgV[4], "--vtk-factory-prefer") > 0 ||
      strcmp(scopedTestCtx.ArgV[5], "Type=Special;Level=2;Type=AnotherSpecial;Level=1") > 0)
    {
      vtkLog(ERROR, "Remaining arguments do not match expected values");
      success = false;
    }
  }
  {
    // same as case 1 but with '=' in the same argument
    vtkLogScopeF(INFO, "Case 2");
    ScopedTestContext scopedTestCtx{ { "ExeName",
      "--vtk-factory-prefer=Type=Special;Level=2;Type=AnotherSpecial;Level=1" } };
    if (!vtkObjectFactory::InitializePreferencesFromCommandLineArgs(
          scopedTestCtx.ArgC, scopedTestCtx.ArgV))
    {
      vtkLog(ERROR, "vtkObjectFactory::InitializePreferencesFromCommandLineArgs returned false");
      success = false;
    }
    if (scopedTestCtx.HasWarnings())
    {
      vtkLog(ERROR, "Unexpected warnings received");
      success = false;
    }
  }
  {
    vtkLogScopeF(INFO, "Case 3");
    ScopedTestContext scopedTestCtx{ { "ExeName", "--vtk-factory-prefer" } };
    if (vtkObjectFactory::InitializePreferencesFromCommandLineArgs(
          scopedTestCtx.ArgC, scopedTestCtx.ArgV))
    {
      vtkLog(ERROR,
        "Expected vtkObjectFactory::InitializePreferencesFromCommandLineArgs to return false");
      success = false;
    }
    if (!scopedTestCtx.WarningMatches("Empty value provided for --vtk-factory-prefer argument."))
    {
      vtkLog(ERROR, "Expected warnings did not occur");
      success = false;
    }
  }
  {
    // same as case 3 but with '=' in the same argument
    vtkLogScopeF(INFO, "Case 4");
    ScopedTestContext scopedTestCtx{ { "ExeName", "--vtk-factory-prefer=" } };
    if (vtkObjectFactory::InitializePreferencesFromCommandLineArgs(
          scopedTestCtx.ArgC, scopedTestCtx.ArgV))
    {
      vtkLog(ERROR,
        "Expected vtkObjectFactory::InitializePreferencesFromCommandLineArgs to return false");
      success = false;
    }
    if (!scopedTestCtx.WarningMatches("Empty value provided for --vtk-factory-prefer argument."))
    {
      vtkLog(ERROR, "Expected warnings did not occur");
      success = false;
    }
  }
  {
    vtkLogScopeF(INFO, "Case 5");
    ScopedTestContext scopedTestCtx{ { "ExeName", "--vtk-factory-prefer",
      "Type;Level=2,Type=AnotherSpecial" } };
    if (vtkObjectFactory::InitializePreferencesFromCommandLineArgs(
          scopedTestCtx.ArgC, scopedTestCtx.ArgV))
    {
      vtkLog(ERROR,
        "Expected vtkObjectFactory::InitializePreferencesFromCommandLineArgs to return false");
      success = false;
    }
    if (!scopedTestCtx.WarningMatches(
          "Invalid format for vtk-factory-prefer: 'Type'. Expected format "
          "'key=value1,value2,...;anotherKey=...'"))
    {
      vtkLog(ERROR, "Expected warnings did not occur");
      success = false;
    }
  }
  {
    // same as case 5 but with '=' in the same argument
    vtkLogScopeF(INFO, "Case 6");
    ScopedTestContext scopedTestCtx{ { "ExeName",
      "--vtk-factory-prefer=Type;Level=2,Type=AnotherSpecial" } };
    if (vtkObjectFactory::InitializePreferencesFromCommandLineArgs(
          scopedTestCtx.ArgC, scopedTestCtx.ArgV))
    {
      vtkLog(ERROR,
        "Expected vtkObjectFactory::InitializePreferencesFromCommandLineArgs to return false");
      success = false;
    }
    if (!scopedTestCtx.WarningMatches(
          "Invalid format for vtk-factory-prefer: 'Type'. Expected format "
          "'key=value1,value2,...;anotherKey=...'"))
    {
      vtkLog(ERROR, "Expected warnings did not occur");
      success = false;
    }
  }
  {
    vtkLogScopeF(INFO, "Case 7");
    ScopedTestContext scopedTestCtx{ { "ExeName", "--vtk-factory-prefer",
      "Type=;Level=2,Type=AnotherSpecial" } };
    if (vtkObjectFactory::InitializePreferencesFromCommandLineArgs(
          scopedTestCtx.ArgC, scopedTestCtx.ArgV))
    {
      vtkLog(ERROR,
        "Expected vtkObjectFactory::InitializePreferencesFromCommandLineArgs to return false");
      success = false;
    }
    if (!scopedTestCtx.WarningMatches("Invalid format for vtk-factory-prefer: 'Type='. At least "
                                      "one value expected for key 'Type'"))
    {
      vtkLog(ERROR, "Expected warnings did not occur");
      success = false;
    }
  }
  {
    // same as case 6 but with '=' in the same argument
    vtkLogScopeF(INFO, "Case 8");
    ScopedTestContext scopedTestCtx{ { "ExeName",
      "--vtk-factory-prefer=Type=;Level=2,Type=AnotherSpecial" } };
    if (vtkObjectFactory::InitializePreferencesFromCommandLineArgs(
          scopedTestCtx.ArgC, scopedTestCtx.ArgV))
    {
      vtkLog(ERROR,
        "Expected vtkObjectFactory::InitializePreferencesFromCommandLineArgs to return false");
      success = false;
    }
    if (!scopedTestCtx.WarningMatches("Invalid format for vtk-factory-prefer: 'Type='. At least "
                                      "one value expected for key 'Type'"))
    {
      vtkLog(ERROR, "Expected warnings did not occur");
      success = false;
    }
  }
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
