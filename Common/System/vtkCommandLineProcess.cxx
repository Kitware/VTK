/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCommandLineProcess.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCommandLineProcess.h"
#include "vtkObjectFactory.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <string>
#include <vector>

namespace details
{
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
static inline void trim(std::string& s)
{
  ltrim(s);
  rtrim(s);
}

std::vector<std::string> ParseCommand(std::string command)
{
  details::trim(command);
  std::vector<std::string> res;

  // Extract program name using regex
  // regex recognize pattern such as `exec`, `./exec`, `/d1/d_2/exec`, `/d1/d\ 2/exec`, or `"/d1/d
  // 2/exec"`. There is only one capturing group and that is the executable without the "
  // characters, when this technique is used to escape the spaces.
  std::regex programRegex =
    std::regex(R"~(^\.?(?:\/?(?:\w+(?:\\ )?)+)+|^"(\.?(?:\/?(?:\w+ ?)+)+)")~");
  std::smatch match;
  if (std::regex_search(command, match, programRegex))
  {
    // If escape group for '"/../..."' matched
    if (match[1].matched)
    {
      res.emplace_back(match[1].str());
    }
    else
    {
      res.emplace_back(match[0].str());
    }
    command = command.substr(match[0].length(), command.length());
  }

  // Extract arguments. Split by space except if surrounded by the '"' character.
  std::regex argRegex = std::regex(R"~([^\s"]+|"([^"]*)")~");
  while (std::regex_search(command, match, argRegex))
  {
    // If escape group for argumeetns with '"' has matched
    if (match[1].matched)
    {
      res.emplace_back(match[1].str());
    }
    else
    {
      res.emplace_back(match[0].str());
    }
    command = command.substr(match[0].length(), command.length());
  }

  return res;
}
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkCommandLineProcess);

//------------------------------------------------------------------------------
vtkCommandLineProcess::vtkCommandLineProcess()
{
  this->SetStdErr("");
  this->SetStdOut("");
}

//------------------------------------------------------------------------------
vtkCommandLineProcess::~vtkCommandLineProcess()
{
  this->SetStdErr(nullptr);
  this->SetStdOut(nullptr);
}

//------------------------------------------------------------------------------
void vtkCommandLineProcess::Execute()
{
  if (this->Command && strlen(this->Command))
  {
    std::vector<std::string> parsed = details::ParseCommand(this->Command);

    // get a vector of C string for vtksys
    const auto size = parsed.size();
    std::vector<const char*> stringViewC(size + 1);
    for (std::size_t i = 0; i < size; ++i)
    {
      stringViewC[i] = parsed[i].c_str();
    }
    stringViewC[size] = nullptr;

    // Configure and launch process
    vtksysProcess* process = vtksysProcess_New();
    vtksysProcess_SetCommand(process, &stringViewC[0]);
    vtksysProcess_SetPipeShared(process, vtksysProcess_Pipe_STDOUT, 0);
    vtksysProcess_SetPipeShared(process, vtksysProcess_Pipe_STDERR, 0);
    vtksysProcess_SetTimeout(process, this->Timeout);
    vtksysProcess_Execute(process);

    // Get ouput streams
    int pipe;
    std::string out;
    std::string err;
    // While loop needed because there is a limit to the buffer size of
    // the vtksysProcess streams. If output is too big we have to append.
    do
    {
      char* cp;
      int length = 0;
      pipe = vtksysProcess_WaitForData(process, &cp, &length, nullptr);
      switch (pipe)
      {
        case vtksysProcess_Pipe_STDOUT:
          out.append(std::string(cp, length));
          break;

        case vtksysProcess_Pipe_STDERR:
          err.append(std::string(cp, length));
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
    this->SetStdOut(out.c_str());
    this->SetStdErr(err.c_str());
  }
}

//------------------------------------------------------------------------------
int vtkCommandLineProcess::ExitProcess(vtksysProcess* process)
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
      vtkDebugMacro("Childs process returned with value: " << code);
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
void vtkCommandLineProcess::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Command: " << this->GetCommand() << std::endl;
  os << indent << "Timeout: " << this->GetTimeout() << std::endl;
  os << indent << "RightTrimResult: " << this->GetRightTrimResult() << std::endl;
}
