/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMultipleInputOutputFilter.cxx
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
#include "vtkImageMultipleInputOutputFilter.h"
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkImageMultipleInputOutputFilter* vtkImageMultipleInputOutputFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageMultipleInputOutputFilter");
  if(ret)
    {
    return (vtkImageMultipleInputOutputFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageMultipleInputOutputFilter;
}



//----------------------------------------------------------------------------
vtkImageMultipleInputOutputFilter::vtkImageMultipleInputOutputFilter()
{
}

//----------------------------------------------------------------------------
vtkImageMultipleInputOutputFilter::~vtkImageMultipleInputOutputFilter()
{      
}

//----------------------------------------------------------------------------
void vtkImageMultipleInputOutputFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageMultipleInputFilter::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageMultipleInputOutputFilter::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageMultipleInputOutputFilter::GetOutput(int idx)
{
  if (this->NumberOfOutputs <= idx)
    {
    return NULL;
    }
  
  return (vtkImageData*)(this->Outputs[idx]);
}


//----------------------------------------------------------------------------
void vtkImageMultipleInputOutputFilter::ExecuteInformation()
{
  vtkImageData *output;
  vtkImageData *input = this->GetInput(0);
  
  if ( input == NULL)
    {
    return;
    }
  
  // Set the defaults from input1 to all outputs
  for (int i = 0; i < this->NumberOfOutputs; i++)
    {
    output = this->GetOutput(i);
    if (output)
      {
      output->CopyTypeSpecificInformation(input);
      }
    }

  // Let the subclass modify the default.
  this->ExecuteInformation((vtkImageData**)(this->Inputs), 
                           (vtkImageData**)(this->Outputs));
}

// Call the alternate version of this method, and use the returned input
// update extent for all inputs
void vtkImageMultipleInputOutputFilter::
ComputeInputUpdateExtents( vtkDataObject *output )
{
  int outExt[6], inExt[6];
  int idx;
  
  output->GetUpdateExtent( outExt );
  
  for (idx = 0; idx < this->NumberOfInputs; idx++)
    {
    if (this->Inputs[idx] != NULL)
      {
      this->ComputeInputUpdateExtent( inExt, outExt, idx );
      this->Inputs[idx]->SetUpdateExtent( inExt );
      }
    }  

  // by default set other output's UpdateExtent to the same if they are unset
  for (idx = 0; idx < this->NumberOfOutputs; idx++)
    {
    if (this->Outputs[idx] && this->Outputs[idx] != output)
      {
      int *uExtent = this->Outputs[idx]->GetUpdateExtent();
      if (uExtent[0] > uExtent[1])
        {
        this->Outputs[idx]->SetUpdateExtent(outExt);
        }
      }
    }
}

// By default, simply set the input update extent to match the given output
// extent
void vtkImageMultipleInputOutputFilter::ComputeInputUpdateExtent( 
  int inExt[6],
  int outExt[6],
  int vtkNotUsed(whichInput) )
{
  memcpy(inExt,outExt,sizeof(int)*6);
}


struct vtkImageMultiThreadStruct
{
  vtkImageMultipleInputOutputFilter *Filter;
  vtkImageData   **Inputs;
  vtkImageData   **Outputs;
};
  
// this mess is really a simple function. All it does is call
// the ThreadedExecute method after setting the correct
// extent for this thread. Its just a pain to calculate
// the correct extent.
VTK_THREAD_RETURN_TYPE vtkImageMultiInOutThreadedExecute( void *arg )
{
  vtkImageMultiThreadStruct *str;
  int ext[6], splitExt[6], total;
  int threadId, threadCount;
  
  threadId = ((ThreadInfoStruct *)(arg))->ThreadID;
  threadCount = ((ThreadInfoStruct *)(arg))->NumberOfThreads;
  
  str = (vtkImageMultiThreadStruct *)(((ThreadInfoStruct *)(arg))->UserData);
  
  memcpy(ext,str->Filter->GetOutput()->GetUpdateExtent(),
	 sizeof(int)*6);

  // execute the actual method with appropriate extent
  // first find out how many pieces extent can be split into.
  total = str->Filter->SplitExtent(splitExt, ext, threadId, threadCount);
    
  if (threadId < total)
    {
    str->Filter->ThreadedExecute(str->Inputs, str->Outputs, splitExt, threadId);
    }
  // else
  //   {
  //   otherwise don't use this thread. Sometimes the threads dont
  //   break up very well and it is just as efficient to leave a 
  //   few threads idle.
  //   }

  return VTK_THREAD_RETURN_VALUE;
}


//----------------------------------------------------------------------------
// The execute method created by the subclass.
void vtkImageMultipleInputOutputFilter::ExecuteData(vtkDataObject *out)
{ 
  vtkImageData *output = vtkImageData::SafeDownCast(out);
  if (!output)
    {
    vtkWarningMacro("ExecuteData called without ImageData output");
    return;
    }
  output->SetExtent(output->GetUpdateExtent());
  output->AllocateScalars();

  vtkImageMultiThreadStruct str;
  
  str.Filter = this;
  str.Inputs = (vtkImageData **)this->Inputs;
  str.Outputs = (vtkImageData **)this->Outputs;
  
  this->Threader->SetNumberOfThreads(this->NumberOfThreads);
  
  // setup threading and the invoke threadedExecute
  this->Threader->SetSingleMethod(vtkImageMultiInOutThreadedExecute, &str);
  this->Threader->SingleMethodExecute();
}

//----------------------------------------------------------------------------
// The execute method created by the subclass.
void vtkImageMultipleInputOutputFilter::
ThreadedExecute(vtkImageData **vtkNotUsed(inData), 
                vtkImageData **vtkNotUsed(outData),
                int extent[6], int vtkNotUsed(threadId))
{
  extent = extent;
  vtkErrorMacro("subclase should override this method!!!");
}

//----------------------------------------------------------------------------
// The execute method created by the subclass.
void vtkImageMultipleInputOutputFilter::
ThreadedExecute(vtkImageData **vtkNotUsed(inData), 
                vtkImageData *vtkNotUsed(outData),
                int extent[6], int vtkNotUsed(threadId))
{
  extent = extent;
  vtkErrorMacro("This method should not be called!");
}













