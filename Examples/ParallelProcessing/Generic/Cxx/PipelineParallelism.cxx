/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PipelineParallelism.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example demonstrates how to write a pipeline parallel application
// with VTK. It creates two parts of a pipeline on a two different
// processors and connects them with ports. The two processes can then
// process the data in a pipeline mode, i.e:
// 1. Consumer asks the producer to start producing data,
// 2. Consumer receives data and starts processing it,
// 3. Producer starts producing new data,
// 4. Goto 2 unless done.
// The pipeline used in this example is as follows:
// rtSource -> OutputPort --- InputPort -> contour -> Render
//        process 0                 process 1
// See pipe1.cxx and pipe2.cxx for the pipelines.

#include "PipelineParallelism.h"

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

  // Assign the functions and execute them
  controller->SetMultipleMethod(0, pipe1, 0);
  controller->SetMultipleMethod(1, pipe2, 0);
  controller->MultipleMethodExecute();

  // Clean-up and exit
  controller->Finalize();
  controller->Delete();

  return 0;
}









