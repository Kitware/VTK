/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMultipleInputOutputFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageMultipleInputOutputFilter.h"

#include "vtkImageData.h"
#include "vtkMultiThreader.h"
#include "vtkObjectFactory.h"


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
  this->Superclass::PrintSelf(os,indent);
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
  
  return static_cast<vtkImageData*>(this->Outputs[idx]);
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
  this->ExecuteInformation(reinterpret_cast<vtkImageData**>(this->Inputs), 
                           reinterpret_cast<vtkImageData**>(this->Outputs));
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
  
  threadId = static_cast<vtkMultiThreader::ThreadInfo *>(arg)->ThreadID;
  threadCount =
    static_cast<vtkMultiThreader::ThreadInfo *>(arg)->NumberOfThreads;
  
  str = static_cast<vtkImageMultiThreadStruct *>(
    static_cast<vtkMultiThreader::ThreadInfo *>(arg)->UserData);
  
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

  // Too many filters have floating point exceptions to execute
  // with empty input/ no request.
  if (this->UpdateExtentIsEmpty(output))
    {
    return;
    }

  output->SetExtent(output->GetUpdateExtent());
  output->AllocateScalars();

  vtkImageMultiThreadStruct str;
  
  str.Filter = this;
  str.Inputs = reinterpret_cast<vtkImageData **>(this->Inputs);
  str.Outputs = reinterpret_cast<vtkImageData **>(this->Outputs);
  
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
  vtkErrorMacro("Subclass should override this method!!!");
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
