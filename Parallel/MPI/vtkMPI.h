/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPI.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkMPI_h
#define vtkMPI_h

#ifndef USE_STDARG
 #define USE_STDARG
#include "vtkParallelMPIModule.h" // For export macro
 #include "mpi.h"
 #undef USE_STDARG
#else
 #include "mpi.h"
#endif

#include "vtkSystemIncludes.h"

class VTKPARALLELMPI_EXPORT vtkMPICommunicatorOpaqueComm
{
public:
  vtkMPICommunicatorOpaqueComm(MPI_Comm* handle = 0);

  MPI_Comm* GetHandle();

  friend class vtkMPICommunicator;
  friend class vtkMPIController;

protected:
  MPI_Comm* Handle;
};

class VTKPARALLELMPI_EXPORT vtkMPICommunicatorReceiveDataInfo
{
public:
  vtkMPICommunicatorReceiveDataInfo()
  {
    this->Handle=0;
  }
  MPI_Datatype DataType;
  MPI_Status Status;
  MPI_Comm* Handle;
};

class VTKPARALLELMPI_EXPORT vtkMPIOpaqueFileHandle
{
public:
  vtkMPIOpaqueFileHandle() : Handle(MPI_FILE_NULL) { }
  MPI_File Handle;
};

//-----------------------------------------------------------------------------
class vtkMPICommunicatorOpaqueRequest
{
public:
  MPI_Request Handle;
};


#endif // vtkMPI_h
// VTK-HeaderTest-Exclude: vtkMPI.h
