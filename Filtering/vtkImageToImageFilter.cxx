/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToImageFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageToImageFilter.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkImageToImageFilter, "1.45");

//----------------------------------------------------------------------------
vtkImageToImageFilter::vtkImageToImageFilter()
{
  this->NumberOfRequiredInputs = 1;
  this->Bypass = 0;
  this->Threader = vtkMultiThreader::New();
  this->NumberOfThreads = this->Threader->GetNumberOfThreads();
}

//----------------------------------------------------------------------------
vtkImageToImageFilter::~vtkImageToImageFilter()
{
  this->Threader->Delete();
}

//----------------------------------------------------------------------------
void vtkImageToImageFilter::PrintSelf(ostream& os, vtkIndent indent)
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
void vtkImageToImageFilter::SetInput(vtkImageData *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageToImageFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkImageData *)(this->Inputs[0]);
}



//----------------------------------------------------------------------------
// This method can be overriden in a subclass to compute the output
// Information: WholeExtent, Spacing, Origin, ScalarType and
// NumberOfScalarComponents.
void vtkImageToImageFilter::ExecuteInformation()
{
  vtkImageData *input = this->GetInput();
  vtkImageData *output = this->GetOutput();

  // Make sure the Input has been set.
  if ( input == NULL || output == NULL)
    {
    vtkErrorMacro(<< "ExecuteInformation: Input is not set.");
    return;
    }

  // Start with some defaults.
  output->CopyTypeSpecificInformation( input );

  // take this opportunity to modify the defaults

  this->ExecuteInformation(input, output);
}

//----------------------------------------------------------------------------
void vtkImageToImageFilter::ExecuteInformation(
           vtkImageData *vtkNotUsed(inData), vtkImageData *vtkNotUsed(outData))
{
}

//----------------------------------------------------------------------------

// Call the alternate version of this method, and use the returned input
// update extent for all inputs
void vtkImageToImageFilter::ComputeInputUpdateExtents( vtkDataObject *output )
{
  int outExt[6], inExt[6];

  output->GetUpdateExtent( outExt );

  if (this->NumberOfInputs)
    {
    this->ComputeInputUpdateExtent( inExt, outExt );
    }

  for (int idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx] != NULL)
      {
      if (this->Inputs[idx]->GetRequestExactExtent())
        {
        int *currentExt = this->Inputs[idx]->GetUpdateExtent();
        for (int i = 0; i < 6; i += 2)
          {
          if (inExt[i] < currentExt[i] ||
              inExt[i+1] > currentExt[i+1])
            {
            this->Inputs[idx]->SetUpdateExtent( inExt );
            break;
            }
          }
        }
      else
        {
        this->Inputs[idx]->SetUpdateExtent( inExt );
        }
      }
    }  
}

// By default, simply set the input update extent to match the given output
// extent
void vtkImageToImageFilter::ComputeInputUpdateExtent( int inExt[6], 
                                                      int outExt[6] )
{
  memcpy(inExt,outExt,sizeof(int)*6);
}




struct vtkImageThreadStruct
{
  vtkImageToImageFilter *Filter;
  vtkImageData   *Input;
  vtkImageData   *Output;
};



// this mess is really a simple function. All it does is call
// the ThreadedExecute method after setting the correct
// extent for this thread. Its just a pain to calculate
// the correct extent.
VTK_THREAD_RETURN_TYPE vtkImageThreadedExecute( void *arg )
{
  vtkImageThreadStruct *str;
  int ext[6], splitExt[6], total;
  int threadId, threadCount;
  vtkImageData *output;

  threadId = ((ThreadInfoStruct *)(arg))->ThreadID;
  threadCount = ((ThreadInfoStruct *)(arg))->NumberOfThreads;

  str = (vtkImageThreadStruct *)(((ThreadInfoStruct *)(arg))->UserData);
  output = str->Output;
  output->GetUpdateExtent( ext );

  // execute the actual method with appropriate extent
  // first find out how many pieces extent can be split into.
  total = str->Filter->SplitExtent(splitExt, ext, threadId, threadCount);
  //total = 1;
  
  if (threadId < total)
    {
    str->Filter->ThreadedExecute(str->Input, str->Output, splitExt, threadId);
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
// For streaming and threads.  Splits output update extent into num pieces.
// This method needs to be called num times.  Results must not overlap for
// consistent starting extent.  Subclass can override this method.
// This method returns the number of peices resulting from a successful split.
// This can be from 1 to "total".  
// If 1 is returned, the extent cannot be split.
int vtkImageToImageFilter::SplitExtent(int splitExt[6], int startExt[6], 
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
    --splitAxis;
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
  int valuesPerThread = (int)ceil(range/(double)total);
  int maxThreadIdUsed = (int)ceil(range/(double)valuesPerThread) - 1;
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
// This is the superclasses style of Execute method.  Convert it into
// an imaging style Execute method.
void vtkImageToImageFilter::ExecuteData(vtkDataObject *out)
{
  vtkImageData *outData = this->AllocateOutputData(out);
  this->MultiThread(this->GetInput(),outData);
}

void vtkImageToImageFilter::MultiThread(vtkImageData *inData,
                                        vtkImageData *outData)
{
  vtkImageThreadStruct str;
  
  str.Filter = this;
  str.Input = inData;
  str.Output = outData;
  
  this->Threader->SetNumberOfThreads(this->NumberOfThreads);
  
  // setup threading and the invoke threadedExecute
  this->Threader->SetSingleMethod(vtkImageThreadedExecute, &str);
  this->Threader->SingleMethodExecute();
}


//----------------------------------------------------------------------------
// The execute method created by the subclass.
void vtkImageToImageFilter::ThreadedExecute(vtkImageData *vtkNotUsed(inData), 
                                            vtkImageData *vtkNotUsed(outData),
                                            int extent[6], int threadId)
{
  extent = extent;
  if (threadId == 0)
    {
    vtkErrorMacro("subclass should override ThreadedExecute!!!");
    }
}

