/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPI.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkMPI_h
#define __vtkMPI_h

#include "mpi.h"

class vtkMPICommunicatorOpaqueComm
{
public:
  vtkMPICommunicatorOpaqueComm();

  MPI_Comm* GetHandle();

  friend class vtkMPICommunicator;
  friend class vtkMPIController;

protected:
  MPI_Comm* Handle;
};


#endif // __vtkMPI_h
