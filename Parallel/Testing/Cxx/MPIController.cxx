/*=========================================================================

  Program:   Visualization Toolkit
  Module:    MPIController.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <mpi.h>

#include "vtkMPIController.h"
#include "vtkProcessGroup.h"

#include "ExerciseMultiProcessController.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int main(int argc, char** argv)
{
  // This is here to avoid false leak messages from vtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);

  VTK_CREATE(vtkMPIController, controller);

  controller->Initialize(&argc, &argv, 1);

  int retval = ExerciseMultiProcessController(controller);

  // The previous run of ExerciseMultiProcessController used the native MPI
  // collective operations.  There is also a second (inefficient) implementation
  // of these within the base vtkCommunicator class.  This hack should force the
  // class to use the VTK implementation.  In practice, the collective
  // operations will probably never be used like this, but this is a convenient
  // place to test for completeness.
  VTK_CREATE(vtkProcessGroup, group);
  group->Initialize(controller);
  vtkSmartPointer<vtkMultiProcessController> genericController;
  genericController.TakeReference(
             controller->vtkMultiProcessController::CreateSubController(group));
  if (!retval)
    {
    retval = ExerciseMultiProcessController(genericController);
    }

  controller->Finalize();

  return retval;
}
