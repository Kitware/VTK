/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMultipleInputFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkImageMultipleInputFilter.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageMultipleInputFilter* vtkImageMultipleInputFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageMultipleInputFilter");
  if(ret)
    {
    return (vtkImageMultipleInputFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageMultipleInputFilter;
}





//----------------------------------------------------------------------------
vtkImageMultipleInputFilter::vtkImageMultipleInputFilter()
{
  this->NumberOfInputs = 0;
  this->NumberOfRequiredInputs = 1;
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
  os << indent << "NumberOfThreads: " << this->NumberOfThreads << "\n";
  if ( this->Bypass )
    {
    os << indent << "Bypass: On\n";
    }
  else
    {
    os << indent << "Bypass: Off\n";
    }
  
  vtkImageSource::PrintSelf(os,indent);
}


//----------------------------------------------------------------------------
// Adds an input to the first null position in the input list.
// Expands the list memory if necessary
void vtkImageMultipleInputFilter::AddInput(vtkImageData *input)
{
  this->vtkProcessObject::AddInput(input);
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
  
  return (vtkImageData*)(this->Inputs[idx]);
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

  this->LegacyHack = 1;
  this->ExecuteImageInformation();
  if (this->LegacyHack)
    {
    vtkWarningMacro("ExecuteImageInformation should be changed to ExecuteInformation(vtkImageData*, vtkImageData*)");
    return;
    }
    
  // Let the subclass modify the default.
  this->ExecuteInformation((vtkImageData**)(this->Inputs), output);
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
void vtkImageMultipleInputFilter::Execute()
{
  vtkImageData *output = this->GetOutput();

  output->SetExtent(output->GetUpdateExtent());
  output->AllocateScalars();
  this->Execute((vtkImageData**)(this->Inputs), output);
}

//----------------------------------------------------------------------------
// The execute method created by the subclass.
void vtkImageMultipleInputFilter::Execute(vtkImageData **inDatas, 
					  vtkImageData *outData)
{
  vtkImageMultiThreadStruct str;
  
  str.Filter = this;
  str.Inputs = inDatas;
  str.Output = outData;
  
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
				  int extent[6], int vtkNotUsed(threadId))
{
  extent = extent;
  vtkErrorMacro("subclase should override this method!!!");
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













