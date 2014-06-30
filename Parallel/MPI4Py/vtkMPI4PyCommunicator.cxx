/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPI4PyCommunicator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMPI4PyCommunicator.h"

#include "vtkObjectFactory.h"
#include "vtkMPICommunicator.h"
#include "vtkMPI.h"

#include <mpi4py/mpi4py.h>

vtkStandardNewMacro(vtkMPI4PyCommunicator);

//----------------------------------------------------------------------------
vtkMPI4PyCommunicator::vtkMPI4PyCommunicator()
{
}

//----------------------------------------------------------------------------
PyObject* vtkMPI4PyCommunicator::ConvertToPython(vtkMPICommunicator* comm)
{
  // Import mpi4py if it does not exist.
  if (!PyMPIComm_New)
    {
    if (import_mpi4py() < 0)
      {
      Py_RETURN_NONE;
      }
    }

  if (!comm || !comm->GetMPIComm())
    {
    Py_RETURN_NONE;
    }

  return PyMPIComm_New(*comm->GetMPIComm()->GetHandle());
}

//----------------------------------------------------------------------------
vtkMPICommunicator* vtkMPI4PyCommunicator::ConvertToVTK(PyObject* comm)
{
  // Import mpi4py if it does not exist.
  if (!PyMPIComm_Get)
    {
    if (import_mpi4py() < 0)
      {
      return NULL;
      }
    }

  if (!comm || !PyObject_TypeCheck(comm, &PyMPIComm_Type))
    {
    return NULL;
    }

  MPI_Comm *mpiComm = PyMPIComm_Get(comm);
  vtkMPICommunicator* vtkComm = vtkMPICommunicator::New();
  vtkMPICommunicatorOpaqueComm opaqueComm(mpiComm);
  if (!vtkComm->InitializeExternal(&opaqueComm))
    {
    vtkComm->Delete();
    return NULL;
    }

  return vtkComm;
}

//----------------------------------------------------------------------------
void vtkMPI4PyCommunicator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
