/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkMPIController.cxx
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
#include "vtkMPIController.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkMPIController* vtkMPIController::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMPIController");
  if(ret)
    {
    return (vtkMPIController*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMPIController;
}





//----------------------------------------------------------------------------
vtkMPIController::vtkMPIController()
{
}

//----------------------------------------------------------------------------
vtkMPIController::~vtkMPIController()
{
}

//----------------------------------------------------------------------------
void vtkMPIController::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMultiProcessController::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkMPIController::Initialize(int argc, char *argv[])
{
  this->Modified();
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &(this->MaximumNumberOfProcesses));
  if (this->MaximumNumberOfProcesses > VTK_MP_CONTROLLER_MAX_PROCESSES)
    {
    vtkWarningMacro("Maximum of " << VTK_MP_CONTROLLER_MAX_PROCESSES);
    this->MaximumNumberOfProcesses = VTK_MP_CONTROLLER_MAX_PROCESSES;
    }
  
  this->NumberOfProcesses = this->MaximumNumberOfProcesses;
  
  MPI_Comm_rank(MPI_COMM_WORLD, &(this->LocalProcessId));  
}
  
//----------------------------------------------------------------------------
// Execute the method set as the SingleMethod on NumberOfThreads threads.
void vtkMPIController::SingleMethodExecute()
{
  if (this->LocalProcessId < this->NumberOfProcesses)
    {
    if (this->SingleMethod)
      {
      (this->SingleMethod)((void *)(this->SingleData) );
      }
    else
      {
      vtkErrorMacro("SingleMethod not set");
      }
    }
  
  MPI_Barrier (MPI_COMM_WORLD);
  // since we expect to call the method only once.
  MPI_Finalize();
}


//----------------------------------------------------------------------------
// Execute the methods set as the MultipleMethods.
void vtkMPIController::MultipleMethodExecute()
{
  int i = this->LocalProcessId;
  
  if (this->LocalProcessId < this->NumberOfProcesses)
    {
    if (this->MultipleMethod[i])
      {
      (this->MultipleMethod[i])((void *)(this->MultipleData[i]) );
      }
    else
      {
      vtkErrorMacro("MultipleMethod " << i << " not set");
      }
    }
  
  MPI_Barrier (MPI_COMM_WORLD);
  // since we expect to call the method only once.
  MPI_Finalize();
}


//----------------------------------------------------------------------------
int vtkMPIController::Send(int *data, int length, int remoteProcessId, 
			    int tag)
{
  MPI_Send(data, length, MPI_INT, remoteProcessId, tag, MPI_COMM_WORLD);
  return 1;
}
//----------------------------------------------------------------------------
int vtkMPIController::Send(unsigned long *data, int length, 
			   int remoteProcessId, int tag)
{
  MPI_Send(data, length, MPI_UNSIGNED_LONG, remoteProcessId, tag, 
	   MPI_COMM_WORLD);
  return 1;  
}
//----------------------------------------------------------------------------
int vtkMPIController::Send(char *data, int length, 
			   int remoteProcessId, int tag)
{
  MPI_Send(data, length, MPI_CHAR, remoteProcessId, tag, MPI_COMM_WORLD);
  return 1;  
}
//----------------------------------------------------------------------------
int vtkMPIController::Send(float *data, int length, 
			   int remoteProcessId, int tag)
{
  MPI_Send(data, length, MPI_FLOAT, remoteProcessId, tag, MPI_COMM_WORLD);
  return 1;  
}



//----------------------------------------------------------------------------
int vtkMPIController::Receive(int *data, int length, int remoteProcessId, 
			      int tag)
{
  MPI_Status status; 

  if (remoteProcessId == VTK_MP_CONTROLLER_ANY_SOURCE)
    {
    remoteProcessId = MPI_ANY_SOURCE;
    }
  MPI_Recv(data, length, MPI_INT, remoteProcessId, tag, MPI_COMM_WORLD,
	   &status);

  // we should really look at status to determine success
  return 1;
}
//----------------------------------------------------------------------------
int vtkMPIController::Receive(unsigned long *data, int length, 
			      int remoteProcessId, int tag)
{
  MPI_Status status; 

  if (remoteProcessId == VTK_MP_CONTROLLER_ANY_SOURCE)
    {
    remoteProcessId = MPI_ANY_SOURCE;
    }
  MPI_Recv(data, length, MPI_UNSIGNED_LONG, remoteProcessId, 
	   tag, MPI_COMM_WORLD, &status);
  // we should really look at status to determine success
  return 1;
}
//----------------------------------------------------------------------------
int vtkMPIController::Receive(char *data, int length, 
			      int remoteProcessId, int tag)
{
  MPI_Status status; 

  if (remoteProcessId == VTK_MP_CONTROLLER_ANY_SOURCE)
    {
    remoteProcessId = MPI_ANY_SOURCE;
    }
    MPI_Recv(data, length, MPI_CHAR, remoteProcessId, tag, MPI_COMM_WORLD,
	   &status);
  // we should really look at status to determine success
  return 1;
}
//----------------------------------------------------------------------------
int vtkMPIController::Receive(float *data, int length, 
			      int remoteProcessId, int tag)
{
  MPI_Status status; 

  if (remoteProcessId == VTK_MP_CONTROLLER_ANY_SOURCE)
    {
    remoteProcessId = MPI_ANY_SOURCE;
    }
  MPI_Recv(data, length, MPI_FLOAT, remoteProcessId, tag, MPI_COMM_WORLD,
	   &status);
  // we should really look at status to determine success
  return 1;
}




