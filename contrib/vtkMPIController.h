/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkMPIController.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkMPIController - Process communication using MPI
// .SECTION Description
// vtkMPIController supplies a minimal set of communication methods as an
// abstract interface through a variety of multi-processing communication
// techniques.  It accepts Sends and Receives as well as implements
// remove method invocations (RMI)
// The "RegisterAndGetGlobalController" ensures that at most one 
// controller exists per process.  In most cases, the controller will
// be created automatically by a higher level object.
// The intent is to generalize this class to have different multiprocessing
// options: Threads, forking processes with shared memory or pipes.
// The initialization is modeled after vtkMultiThreader, and may merge with
// vtkMultiThreader in the future.

// .SECTION see also
// vtkDownStreamPort vtkUpStreamPort vtkMultiThreader vtkMultiProcessController

#ifndef __vtkMPIController_h
#define __vtkMPIController_h

#include "vtkObject.h"
#include "vtkMultiProcessController.h"
#include "mpi.h"


class VTK_EXPORT vtkMPIController : public vtkMultiProcessController
{
public:
  static vtkMPIController *New();
  vtkTypeMacro(vtkMPIController,vtkMultiProcessController);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method is for setting up the processes.
  void Initialize(int argc, char *arcv[]);

  // Description:
  // Execute the SingleMethod (as define by SetSingleMethod) using
  // this->NumberOfProcesses processes.  You should not expect this to return.
  void SingleMethodExecute();
  
  // Description:
  // Execute the MultipleMethods (as define by calling SetMultipleMethod
  // for each of the required this->NumberOfProcesses methods) using
  // this->NumberOfProcesses processes.
  void MultipleMethodExecute();
  
  //------------------ Communication --------------------
  
  // Description:
  // This method sends data to another process.  Tag eliminates ambiguity
  // when multiple sends ar receives exist in the same process.
  int Send(int *data, int length, int remoteProcessId, int tag);
  int Send(unsigned long *data, int length, int remoteProcessId, int tag);
  int Send(char *data, int length, int remoteProcessId, int tag);
  int Send(float *data, int length, int remoteProcessId, int tag);
  int Send(vtkObject *data, int remoteId, int tag)
    {return this->vtkMultiProcessController::Send(data,remoteId,tag);}

  // Description:
  // This method receives data from a corresponding send. It blocks
  // until the receive is finished.  It calls methods in "data"
  // to communicate the sending data.
  int Receive(int *data, int length, int remoteProcessId, int tag);
  int Receive(unsigned long *data, int length, int remoteProcessId, int tag);
  int Receive(char *data, int length, int remoteProcessId, int tag);
  int Receive(float *data, int length, int remoteProcessId, int tag);
  int Receive(vtkObject *data, int remoteId, int tag)
    {return this->vtkMultiProcessController::Receive(data, remoteId, tag);}

protected:

  vtkMPIController();
  ~vtkMPIController();
  vtkMPIController(const vtkMPIController&) {};
  void operator=(const vtkMPIController&) {};

  // Initialize only once, finialize on destruction.
  int Initialized;
};


#endif


