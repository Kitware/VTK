/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageStencilSource.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G Gobbi who developed this class.

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

#include <math.h>
#include "vtkImageStencilSource.h"
#include "vtkObjectFactory.h"


//----------------------------------------------------------------------------
vtkImageStencilSource* vtkImageStencilSource::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageStencilSource");
  if(ret)
    {
    return (vtkImageStencilSource*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageStencilSource;
}

//----------------------------------------------------------------------------
vtkImageStencilSource::vtkImageStencilSource()
{
  this->vtkSource::SetNthOutput(0,vtkImageStencilData::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
}

//----------------------------------------------------------------------------
vtkImageStencilSource::~vtkImageStencilSource()
{
}

//----------------------------------------------------------------------------
void vtkImageStencilSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkSource::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkImageStencilSource::SetOutput(vtkImageStencilData *output)
{
  this->vtkSource::SetNthOutput(0, output);
}

//----------------------------------------------------------------------------
vtkImageStencilData *vtkImageStencilSource::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return (vtkImageStencilData *)(this->Outputs[0]);
}

//----------------------------------------------------------------------------
vtkImageStencilData * 
vtkImageStencilSource::AllocateOutputData(vtkDataObject *out)
{
  vtkImageStencilData *res = vtkImageStencilData::SafeDownCast(out);
  if (!res)
    {
    vtkWarningMacro("Call to AllocateOutputData with non vtkImageStencilData"
                    " output");
    return NULL;
    }
  res->SetExtent(res->GetUpdateExtent());
  res->SetOldSpacing(res->GetSpacing());
  res->SetOldOrigin(res->GetOrigin());
  res->AllocateExtents();

  return res;
}  

//----------------------------------------------------------------------------
void vtkImageStencilSource::ExecuteData(vtkDataObject *out)
{
  vtkImageStencilData *output = this->AllocateOutputData(out);

  // no multithreading yet...
  this->ThreadedExecute(output, output->GetExtent(), 0);
}

//----------------------------------------------------------------------------
void vtkImageStencilSource::ThreadedExecute(vtkImageStencilData *vtkNotUsed(o),
                                            int extent[6], int threadId)
{
  extent = extent; // this silly line avoids compiler warnings
  if (threadId == 0)
    {
    vtkErrorMacro("subclass should override ThreadedExecute!!!");
    }
}


