/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPIEventLog.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkMPIEventLog.h"
#include "vtkMPIController.h"
#include "vtkObjectFactory.h"
#include "mpe.h"

int vtkMPIEventLog::LastEventId = 0;

vtkMPIEventLog* vtkMPIEventLog::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMPIEventLog");
  if(ret)
    {
    return (vtkMPIEventLog*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMPIEventLog;
}

void vtkMPIEventLog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os,indent);
}

vtkMPIEventLog::vtkMPIEventLog()
{

  this->Active = 0;
  
}

void vtkMPIEventLog::InitializeLogging()
{
  MPE_Init_log();
}

void vtkMPIEventLog::FinalizeLogging(const char* fname)
{
  MPE_Finish_log(const_cast<char*>(fname));
}

int vtkMPIEventLog::SetDescription(const char* name, const char* desc)
{
  int err, processId;
  if ( (err = MPI_Comm_rank(MPI_COMM_WORLD,&processId)) 
       != MPI_SUCCESS)
    {
    char *msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occured: " << msg);
    delete[] msg;
    return 0;
    }

  this->Active = 1;
  if (processId == 0)
    {
    this->BeginId = MPE_Log_get_event_number();
    this->EndId = MPE_Log_get_event_number();
    MPE_Describe_state(this->BeginId, this->EndId, const_cast<char*>(name), 
		       const_cast<char*>(desc));
    }
  MPI_Bcast(&this->BeginId, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&this->EndId, 1, MPI_INT, 0, MPI_COMM_WORLD);

  return 1;
} 

void vtkMPIEventLog::StartLogging()
{
  if (!this->Active)
    {
    vtkWarningMacro("This vtkMPIEventLog has not been initialized. Can not log event.");
    return;
    }

  MPE_Log_event(this->BeginId, 0, "begin");
}

void vtkMPIEventLog::StopLogging()
{
  if (!this->Active)
    {
    vtkWarningMacro("This vtkMPIEventLog has not been initialized. Can not log event.");
    return;
    }
  MPE_Log_event(this->EndId, 0, "end");
}

vtkMPIEventLog::~vtkMPIEventLog()
{
}

