// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkExecutableRunner.h"
#include "vtkObjectFactory.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>
#include <vector>

namespace details
{
VTK_ABI_NAMESPACE_BEGIN
// trim strings. This should go in vtksys at some point
static inline void ltrim(std::string& s)
{
  s.erase(s.begin(),
    std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
}
static inline void rtrim(std::string& s)
{
  s.erase(
    std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(),
    s.end());
}
VTK_ABI_NAMESPACE_END
}

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkExecutableRunner);

//------------------------------------------------------------------------------
void vtkExecutableRunner::Execute()
{
  std::string command = this->Command;
  details::ltrim(command);
  if (!command.empty())
  {
    std::vector<std::string> splittedCommand = this->GetCommandToExecute();

    // get a vector of C string for vtksys
    std::vector<const char*> stringViewC;
    stringViewC.reserve(splittedCommand.size() + 1);
    for (const auto& cmd : splittedCommand)
    {
      stringViewC.emplace_back(cmd.c_str());
    }
    stringViewC.emplace_back(nullptr);

    // Configure and launch process
    vtksysProcess* process = vtksysProcess_New();
    vtksysProcess_SetCommand(process, stringViewC.data());
    vtksysProcess_SetPipeShared(process, vtksysProcess_Pipe_STDOUT, 0);
    vtksysProcess_SetPipeShared(process, vtksysProcess_Pipe_STDERR, 0);
    vtksysProcess_SetTimeout(process, this->Timeout);
    vtksysProcess_Execute(process);

    // Get output streams
    int pipe;
    std::string out;
    std::string err;
    // While loop needed because there is a limit to the buffer size of
    // the vtksysProcess streams. If output is too big we have to append.
    do
    {
      char* output = nullptr;
      int length = 0;
      pipe = vtksysProcess_WaitForData(process, &output, &length, nullptr);
      switch (pipe)
      {
        case vtksysProcess_Pipe_STDOUT:
          out.append(std::string(output, length));
          break;

        case vtksysProcess_Pipe_STDERR:
          err.append(std::string(output, length));
          break;
      }
    } while (pipe != vtksysProcess_Pipe_None);

    // Exit properly
    this->ReturnValue = this->ExitProcess(process);
    vtksysProcess_Delete(process);

    // Trim last whitespaces
    if (this->RightTrimResult)
    {
      details::rtrim(out);
      details::rtrim(err);
    }
    this->SetStdOut(out);
    this->SetStdErr(err);
  }
}

//------------------------------------------------------------------------------
int vtkExecutableRunner::ExitProcess(vtksysProcess* process)
{
  vtksysProcess_WaitForExit(process, &this->Timeout);

  int state = vtksysProcess_GetState(process);
  int code = -1;
  switch (state)
  {
    case vtksysProcess_State_Error:
      vtkErrorMacro(
        "Child process administration error: " << vtksysProcess_GetErrorString(process));
      break;
    case vtksysProcess_State_Exception:
      vtkErrorMacro(
        "Child process exited abnormally: " << vtksysProcess_GetExceptionString(process));
      break;
    case vtksysProcess_State_Expired:
      vtkErrorMacro("Child process's timeout expired.");
      break;
    case vtksysProcess_State_Killed:
      vtkErrorMacro("Child process terminated by Kill method.");
      break;
    case vtksysProcess_State_Exited:
      code = vtksysProcess_GetExitValue(process);
      vtkDebugMacro("Child process returned with value: " << code);
      if (code)
      {
        vtkWarningMacro("Child process exited with error code: " << code);
      }
      break;

    default:
      break;
  }

  return code;
}

//------------------------------------------------------------------------------
std::vector<std::string> vtkExecutableRunner::GetCommandToExecute() const
{
  std::vector<std::string> result;
  if (this->ExecuteInSystemShell)
  {
    result.reserve(3);
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    result.emplace_back("cmd.exe");
    result.emplace_back("/c");
#else
    result.emplace_back("sh");
    result.emplace_back("-c");
#endif
    result.emplace_back(this->Command);
  }
  else
  {
    result.reserve(this->Arguments.size() + 1);
    result.emplace_back(this->Command);
    for (const auto& arg : this->Arguments)
    {
      result.emplace_back(arg);
    }
  }

  return result;
}

//------------------------------------------------------------------------------
void vtkExecutableRunner::AddArgument(const std::string& arg)
{
  this->Arguments.emplace_back(arg);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkExecutableRunner::ClearArguments()
{
  if (!this->Arguments.empty())
  {
    this->Arguments.clear();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkExecutableRunner::GetNumberOfArguments() const
{
  return this->Arguments.size();
}

//------------------------------------------------------------------------------
void vtkExecutableRunner::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Command: " << this->GetCommand() << std::endl;
  os << indent << "Timeout: " << this->GetTimeout() << std::endl;
  os << indent << "RightTrimResult: " << this->GetRightTrimResult() << std::endl;
}
VTK_ABI_NAMESPACE_END
