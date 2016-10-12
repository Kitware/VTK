/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPSystemTools.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPSystemTools.h"

#include <vtkMultiProcessController.h>
#include "vtkObjectFactory.h"
#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkPSystemTools);

//----------------------------------------------------------------------------
void vtkPSystemTools::BroadcastString(std::string& str, int proc)
{
  vtkMultiProcessController* controller =
    vtkMultiProcessController::GetGlobalController();

  vtkIdType size = static_cast<vtkIdType>(str.size());
  controller->Broadcast(&size, 1, proc);

  str.resize(size);
  if(size)
  {
    controller->Broadcast(&str[0], size, proc);
  }
}

//----------------------------------------------------------------------------
std::string vtkPSystemTools::CollapseFullPath(const std::string& in_relative)
{
  vtkMultiProcessController* controller =
    vtkMultiProcessController::GetGlobalController();
  std::string returnString;
  if(controller->GetLocalProcessId() == 0)
  {
    returnString = vtksys::SystemTools::CollapseFullPath(in_relative, 0);
  }
  vtkPSystemTools::BroadcastString(returnString, 0);

  return returnString;
}

//----------------------------------------------------------------------------
std::string vtkPSystemTools::CollapseFullPath(const std::string& in_path,
                                            const char* in_base)
{
  vtkMultiProcessController* controller =
    vtkMultiProcessController::GetGlobalController();
  std::string returnString;
  if(controller->GetLocalProcessId() == 0)
  {
    returnString = vtksys::SystemTools::CollapseFullPath(in_path, in_base);
  }
  vtkPSystemTools::BroadcastString(returnString, 0);

  return returnString;
}

//----------------------------------------------------------------------------
bool vtkPSystemTools::FileExists(const char* filename)
{
  if(!filename)
  {
    return false;
  }
  return vtkPSystemTools::FileExists(std::string(filename));
}

//----------------------------------------------------------------------------
bool vtkPSystemTools::FileExists(const std::string& filename)
{
  if(filename.empty())
  {
    return false;
  }
  vtkMultiProcessController* controller =
    vtkMultiProcessController::GetGlobalController();
  int exists = 0;
  if(controller->GetLocalProcessId() == 0)
  {
    exists = vtksys::SystemTools::FileExists(filename);
  }
  controller->Broadcast(&exists, 1, 0);
  return exists != 0;
}

//----------------------------------------------------------------------------
bool vtkPSystemTools::FileExists(const char* filename, bool isFile)
{
  if(!filename)
  {
    return false;
  }
  return vtkPSystemTools::FileExists(std::string(filename), isFile);
}

//----------------------------------------------------------------------------
bool vtkPSystemTools::FileExists(const std::string& filename, bool isFile)
{
  vtkMultiProcessController* controller =
    vtkMultiProcessController::GetGlobalController();
  int exists = 0;
  if(controller->GetLocalProcessId() == 0)
  {
    exists = vtksys::SystemTools::FileExists(filename, isFile);
  }
  controller->Broadcast(&exists, 1, 0);
  return exists != 0;
}

//----------------------------------------------------------------------------
bool vtkPSystemTools::FileIsDirectory(const std::string& inName)
{
  vtkMultiProcessController* controller =
    vtkMultiProcessController::GetGlobalController();
  int isDirectory = 0;
  if(controller->GetLocalProcessId() == 0)
  {
    isDirectory = vtksys::SystemTools::FileIsDirectory(inName);
  }
  controller->Broadcast(&isDirectory, 1, 0);
  return isDirectory != 0;
}

//----------------------------------------------------------------------------
bool vtkPSystemTools::FindProgramPath(const char* argv0,
                                    std::string& pathOut,
                                    std::string& errorMsg,
                                    const char* exeName,
                                    const char* buildDir,
                                    const char* installPrefix )
{
  vtkMultiProcessController* controller =
    vtkMultiProcessController::GetGlobalController();
  int retVal = 1;
  if(controller->GetLocalProcessId() == 0)
  {
    retVal = static_cast<int>(
      vtksys::SystemTools::FindProgramPath(
        argv0, pathOut, errorMsg, exeName, buildDir, installPrefix));
  }
  controller->Broadcast(&retVal, 1, 0);
  // if the retVal on proc 0 is non-zero then only information is
  // put in pathOut. Otherwise information is put in errorMsg.
  if(retVal)
  {
    vtkPSystemTools::BroadcastString(pathOut, 0);
  }
  else
  {
    vtkPSystemTools::BroadcastString(errorMsg, 0);
  }
  return retVal != 0;
}

//----------------------------------------------------------------------------
std::string vtkPSystemTools::GetCurrentWorkingDirectory(bool collapse)
{
  vtkMultiProcessController* controller =
    vtkMultiProcessController::GetGlobalController();
  std::string returnString;
  if(controller->GetLocalProcessId() == 0)
  {
    returnString = vtksys::SystemTools::GetCurrentWorkingDirectory(collapse);
  }
  vtkPSystemTools::BroadcastString(returnString, 0);
  return returnString;
}

//----------------------------------------------------------------------------
std::string vtkPSystemTools::GetProgramPath(const std::string& path)
{
  vtkMultiProcessController* controller =
    vtkMultiProcessController::GetGlobalController();
  std::string programPath;
  if(controller->GetLocalProcessId() == 0)
  {
    programPath = vtksys::SystemTools::GetProgramPath(path);
  }
  vtkPSystemTools::BroadcastString(programPath, 0);

  return programPath;
}

//----------------------------------------------------------------------------
void vtkPSystemTools::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
