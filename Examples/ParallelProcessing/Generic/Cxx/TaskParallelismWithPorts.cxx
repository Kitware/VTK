/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TaskParallelismWithPorts.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example demonstrates how to write a task parallel application
// with VTK. It creates two different pipelines and assigns each to
// one processor. These pipelines are:
// 1. rtSource -> contour            -> probe               .-> append
//             \                     /                  port
//              -> gradient magnitude                  /
// 2. rtSource -> gradient -> shrink -> glyph3D -> port
// See task3.cxx and task4.cxx for the pipelines.

#include "TaskParallelismWithPorts.h"

// This function sets up properties common to both processes
// and executes the task corresponding to the current process
void process(vtkMultiProcessController* controller, void* vtkNotUsed(arg))
{
  taskFunction task;
  int myId = controller->GetLocalProcessId();

  // Chose the appropriate task (see task3.cxx and task4.cxx)
  if ( myId == 0 )
    {
    task = task3;
    }
  else
    {
    task = task4;
    }

  // Run the tasks (see task3.cxx and task4.cxx)
  (*task)(EXTENT);
}


int main( int argc, char* argv[] )
{
  
  // Note that this will create a vtkMPIController if MPI
  // is configured, vtkThreadedController otherwise.
  vtkMultiProcessController* controller = vtkMultiProcessController::New();
  controller->Initialize(&argc, &argv);

  // When using MPI, the number of processes is determined
  // by the external program which launches this application.
  // However, when using threads, we need to set it ourselves.
  if (controller->IsA("vtkThreadedController"))
    {
    // Set the number of processes to 2 for this example.
    controller->SetNumberOfProcesses(2);
    } 
  int numProcs = controller->GetNumberOfProcesses();

  if (numProcs != 2)
    {
    cerr << "This example requires two processes." << endl;
    controller->Finalize();
    controller->Delete();
    return 1;
    }


  // Execute the function named "process" on both processes
  controller->SetSingleMethod(process, 0);
  controller->SingleMethodExecute();
  
  // Clean-up and exit
  controller->Finalize();
  controller->Delete();
  
  return 0;
}









