/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMultipleInputFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageMultipleInputFilter.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkMultiThreader.h"
#include "vtkObjectFactory.h"


//----------------------------------------------------------------------------
vtkImageMultipleInputFilter::vtkImageMultipleInputFilter()
{
  this->NumberOfInputs = 0;
  this->NumberOfRequiredInputs = 1;
  this->SetNumberOfInputPorts(1);
  this->Bypass = 0;

  this->Threader = vtkMultiThreader::New();
  this->NumberOfThreads = this->Threader->GetNumberOfThreads();
}

//----------------------------------------------------------------------------
vtkImageMultipleInputFilter::~vtkImageMultipleInputFilter()
{      
  this->Threader->Delete();
}

//----------------------------------------------------------------------------
void vtkImageMultipleInputFilter::PrintSelf(ostream& os, vtkIndent indent)
{  
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "NumberOfThreads: " << this->NumberOfThreads << "\n";
  if ( this->Bypass )
    {
    os << indent << "Bypass: On\n";
    }
  else
    {
    os << indent << "Bypass: Off\n";
    }
}


//----------------------------------------------------------------------------
// Adds an input to the first null position in the input list.
// Expands the list memory if necessary
void vtkImageMultipleInputFilter::AddInput(vtkImageData *input)
{
  this->vtkProcessObject::AddInput(input);
}

//----------------------------------------------------------------------------
void vtkImageMultipleInputFilter::RemoveInput(vtkImageData *input)
{
  this->vtkProcessObject::RemoveInput(input);
}

//----------------------------------------------------------------------------
// Set an Input of this filter. 
void vtkImageMultipleInputFilter::SetInput(int idx, vtkImageData *input)
{
  this->vtkProcessObject::SetNthInput(idx, input);
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageMultipleInputFilter::GetInput()
{
  return this->GetInput(0);
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageMultipleInputFilter::GetInput(int idx)
{
  if (this->NumberOfInputs <= idx)
    {
    return NULL;
    }
  
  return static_cast<vtkImageData *>(this->Inputs[idx]);
}

//----------------------------------------------------------------------------
void vtkImageMultipleInputFilter::ExecuteInformation()
{
  vtkImageData *output = this->GetOutput();
  vtkImageData *input = this->GetInput(0);
  
  if ( input == NULL || output == NULL)
    {
    return;
    }
  
  // Set the defaults from input1
  output->CopyTypeSpecificInformation(input);

  // Let the subclass modify the default.
  this->ExecuteInformation(reinterpret_cast<vtkImageData**>(this->Inputs),
                           output);
}

// Call the alternate version of this method, and use the returned input
// update extent for all inputs
void vtkImageMultipleInputFilter::ComputeInputUpdateExtents( vtkDataObject 
                                                             *output )
{
  int outExt[6], inExt[6];

  output->GetUpdateExtent( outExt );

  for (int idx = 0; idx < this->NumberOfInputs; idx++)
    {
    if (this->Inputs[idx] != NULL)
      {
      this->ComputeInputUpdateExtent( inExt, outExt, idx );
      this->Inputs[idx]->SetUpdateExtent( inExt );
      }
    }  
}

// By default, simply set the input update extent to match the given output
// extent
void vtkImageMultipleInputFilter::ComputeInputUpdateExtent( 
                                            int inExt[6],
                                            int outExt[6],
                                            int vtkNotUsed(whichInput) )
{
  memcpy(inExt,outExt,sizeof(int)*6);
}


struct vtkImageMultiThreadStruct
{
  vtkImageMultipleInputFilter *Filter;
  vtkImageData   **Inputs;
  vtkImageData   *Output;
};
  
// this mess is really a simple function. All it does is call
// the ThreadedExecute method after setting the correct
// extent for this thread. Its just a pain to calculate
// the correct extent.
VTK_THREAD_RETURN_TYPE vtkImageMultiThreadedExecute( void *arg )
{
  vtkImageMultiThreadStruct *str;
  int ext[6], splitExt[6], total;
  int threadId, threadCount;
  
  threadId = static_cast<vtkMultiThreader::ThreadInfo *>(arg)->ThreadID;
  threadCount =
    static_cast<vtkMultiThreader::ThreadInfo *>(arg)->NumberOfThreads;
  
  str =static_cast<vtkImageMultiThreadStruct *>(
    static_cast<vtkMultiThreader::ThreadInfo *>(arg)->UserData);
  
  memcpy(ext,str->Filter->GetOutput()->GetUpdateExtent(),
         sizeof(int)*6);

  // execute the actual method with appropriate extent
  // first find out how many pieces extent can be split into.
  total = str->Filter->SplitExtent(splitExt, ext, threadId, threadCount);
    
  if (threadId < total)
    {
    str->Filter->ThreadedExecute(str->Inputs, str->Output, splitExt, threadId);
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
void vtkImageMultipleInputFilter::ExecuteData(vtkDataObject *out)
{
  // Make sure the Input has been set.
  if ( this->GetInput() == NULL )
    {
    vtkErrorMacro(<< "ExecuteData: Input is not set.");
    return;
    }

  // Too many filters have floating point exceptions to execute
  // with empty input/ no request.
  if (this->UpdateExtentIsEmpty(out))
    {
    return;
    }


  vtkImageData *outdata = this->AllocateOutputData(out);
  this->MultiThread(reinterpret_cast<vtkImageData**>(this->GetInputs()),
                    outdata);
}

//----------------------------------------------------------------------------
void vtkImageMultipleInputFilter::MultiThread(vtkImageData **inputs,
                                              vtkImageData *output)
{
  vtkImageMultiThreadStruct str;
  
  str.Filter = this;
  str.Inputs = inputs;
  str.Output = output;
  
  this->Threader->SetNumberOfThreads(this->NumberOfThreads);
  
  // setup threading and the invoke threadedExecute
  this->Threader->SetSingleMethod(vtkImageMultiThreadedExecute, &str);
  this->Threader->SingleMethodExecute();
}

//----------------------------------------------------------------------------
// The execute method created by the subclass.
void vtkImageMultipleInputFilter::ThreadedExecute(vtkImageData 
                                  **vtkNotUsed(inData), 
                                  vtkImageData *vtkNotUsed(outData),
                                  int extent[6], int threadId)
{
  extent = extent;
  if (threadId == 0)
    {
    vtkErrorMacro("subclass must override ThreadedExecute!!!");
    }
}

  
//----------------------------------------------------------------------------
// For streaming and threads.  Splits output update extent into num pieces.
// This method needs to be called num times.  Results must not overlap for
// consistent starting extent.  Subclass can override this method.
// This method returns the number of peices resulting from a successful split.
// This can be from 1 to "total".  
// If 1 is returned, the extent cannot be split.
int vtkImageMultipleInputFilter::SplitExtent(int splitExt[6], int startExt[6], 
                                             int num, int total)
{
  int splitAxis;
  int min, max;

  vtkDebugMacro("SplitExtent: ( " << startExt[0] << ", " << startExt[1] << ", "
                << startExt[2] << ", " << startExt[3] << ", "
                << startExt[4] << ", " << startExt[5] << "), " 
                << num << " of " << total);

  // start with same extent
  memcpy(splitExt, startExt, 6 * sizeof(int));

  splitAxis = 2;
  min = startExt[4];
  max = startExt[5];
  while (min == max)
    {
    splitAxis--;
    if (splitAxis < 0)
      { // cannot split
      vtkDebugMacro("  Cannot Split");
      return 1;
      }
    min = startExt[splitAxis*2];
    max = startExt[splitAxis*2+1];
    }

  // determine the actual number of pieces that will be generated
  int range = max - min + 1;
  int valuesPerThread =
    static_cast<int>(ceil(range/static_cast<double>(total)));
  int maxThreadIdUsed =
    static_cast<int>(ceil(range/static_cast<double>(valuesPerThread)) - 1);
  if (num < maxThreadIdUsed)
    {
    splitExt[splitAxis*2] = splitExt[splitAxis*2] + num*valuesPerThread;
    splitExt[splitAxis*2+1] = splitExt[splitAxis*2] + valuesPerThread - 1;
    }
  if (num == maxThreadIdUsed)
    {
    splitExt[splitAxis*2] = splitExt[splitAxis*2] + num*valuesPerThread;
    }
  
  vtkDebugMacro("  Split Piece: ( " <<splitExt[0]<< ", " <<splitExt[1]<< ", "
                << splitExt[2] << ", " << splitExt[3] << ", "
                << splitExt[4] << ", " << splitExt[5] << ")");

  return maxThreadIdUsed + 1;
}

//----------------------------------------------------------------------------
int vtkImageMultipleInputFilter::FillInputPortInformation(int port,
                                                          vtkInformation* info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}
