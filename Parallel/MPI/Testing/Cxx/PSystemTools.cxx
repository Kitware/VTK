/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PSystemTools.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <mpi.h>

#include "vtkMPIController.h"
#include "vtkPSystemTools.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int PSystemTools(int argc, char* argv[])
{
  // This is here to avoid false leak messages from vtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);

  VTK_CREATE(vtkMPIController, controller);

  controller->Initialize(&argc, &argv, 1);
  controller->SetGlobalController(controller);

  int retVal = 0; // success

  std::string str;
  if(controller->GetLocalProcessId() == 0)
  {
    str = "test";
  }
  vtkPSystemTools::BroadcastString(str, 0);
  if(str != "test")
  {
    vtkGenericWarningMacro("BroadcastString failed for process " <<
                           controller->GetLocalProcessId());
    retVal++;
  }

  str = vtkPSystemTools::GetCurrentWorkingDirectory();
  std::string substr = str.substr(str.size()-24, str.size());
  if(substr != "Parallel/MPI/Testing/Cxx")
  {
    vtkGenericWarningMacro("GetCurrentWorkingDirectory failed for process " <<
                           controller->GetLocalProcessId());
    retVal++;
  }

  if(!vtkPSystemTools::FileIsDirectory(str))
  {
    vtkGenericWarningMacro("FileIsDirectory failed for process " <<
                           controller->GetLocalProcessId());
    retVal++;
  }

  str += "/cmake_install.cmake";
  if(!vtkPSystemTools::FileExists(str))
  {
    vtkGenericWarningMacro("FileExists failed for process " <<
                           controller->GetLocalProcessId());
    retVal++;
  }


  controller->SetGlobalController(NULL);
  controller->Finalize();

  return retVal;
}
