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


//----------------------------------------------------------------------------
vtkImageMultipleInputFilter::vtkImageMultipleInputFilter()
{
  this->NumberOfInputs = 0;
  this->Inputs = NULL;
  this->Bypass = 0;
  this->Updating = 0;

  this->Threader = vtkMultiThreader::New();
  this->NumberOfThreads = this->Threader->GetNumberOfThreads();
}

//----------------------------------------------------------------------------
vtkImageMultipleInputFilter::~vtkImageMultipleInputFilter()
{
  int idx;
  
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx])
      {
      this->Inputs[idx]->UnRegister(this);
      this->Inputs[idx] = NULL;
      }
    }
      
  this->Threader->Delete();
  if (this->Inputs)
    {
    delete [] this->Inputs;
    }
}

//----------------------------------------------------------------------------
void vtkImageMultipleInputFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  
  os << indent << "NumberOfThreads: " << this->NumberOfThreads << "\n";
  os << indent << "Bypass: " << this->Bypass << "\n";
  
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    os << indent << "Input " << idx << ": (" << this->Inputs[idx] << ")\n";
    }

  vtkImageSource::PrintSelf(os,indent);
}

typedef vtkImageCache *vtkImageCachePointer;
//----------------------------------------------------------------------------
// Called by constructor to set up input array.
void vtkImageMultipleInputFilter::SetNumberOfInputs(int num)
{
  int idx;
  vtkImageCachePointer *inputs;

  // in case nothing has changed.
  if (num == this->NumberOfInputs)
    {
    return;
    }
  
  // Allocate new arrays.
  inputs = new vtkImageCachePointer[num];

  // Initialize with NULLs.
  for (idx = 0; idx < num; ++idx)
    {
    inputs[idx] = NULL;
    }

  // Copy old inputs
  for (idx = 0; idx < num && idx < this->NumberOfInputs; ++idx)
    {
    inputs[idx] = this->Inputs[idx];
    }
  
  // delete the previous arrays
  if (this->Inputs)
    {
    delete [] this->Inputs;
    }
  
  // Set the new arrays
  this->Inputs = inputs;
  
  this->NumberOfInputs = num;
  this->Modified();
}

//----------------------------------------------------------------------------
// This Method returns the MTime of the pipeline upto and including this filter
// Note: current implementation may create a cascade of GetPipelineMTime calls.
// Each GetPipelineMTime call propagates the call all the way to the original
// source.  This works, but is not elegant.
// An Executor would probably be the best solution if this is a problem.
// (The pipeline could vote before it starts processing, but one object
// has to initiate the voting.)
unsigned long int vtkImageMultipleInputFilter::GetPipelineMTime()
{
  unsigned long int time1, time2;
  int idx;

  // This objects MTime
  // (Super class considers cache in case cache did not originate message)
  time1 = this->vtkImageSource::GetPipelineMTime();
  // Look at input modified times.
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if ( ! this->Inputs[idx])
      {
      time2 = time1;
      }
    else
      {
      time2 = this->Inputs[idx]->GetPipelineMTime();
      }
    
    // Keep the larger of the two 
    if (time2 > time1)
      {
      time1 = time2;
      }
    }
  
  return time1;
}


//----------------------------------------------------------------------------
// Adds an input to the first null position in the input list.
// Expands the list memory if necessary
void vtkImageMultipleInputFilter::AddInput(vtkImageCache *input)
{
  int idx;
  
  if (input)
    {
    input->Register(this);
    }
  this->Modified();
  
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx] == NULL)
      {
      this->Inputs[idx] == input;
      return;
      }
    }
  
  this->SetNumberOfInputs(this->NumberOfInputs + 1);
  this->Inputs[this->NumberOfInputs - 1] = input;
}

//----------------------------------------------------------------------------
// Set an Input of this filter. 
void vtkImageMultipleInputFilter::SetInput(int idx, vtkImageCache *input)
{
  if (idx < 0)
    {
    vtkErrorMacro(<< "SetInput: " << idx << ", cannot set input. ");
    return;
    }
  // Expand array if necessary.
  if (idx >= this->NumberOfInputs)
    {
    this->SetNumberOfInputs(idx + 1);
    }
  
  // does this change anything?
  if (input == this->Inputs[idx])
    {
    return;
    }
  
  if (this->Inputs[idx])
    {
    this->Inputs[idx]->UnRegister(this);
    this->Inputs[idx] = NULL;
    }
  
  if (input)
    {
    input->Register(this);
    }

  this->Inputs[idx] = input;
  this->Modified();
}




//----------------------------------------------------------------------------
// Called by cache
void vtkImageMultipleInputFilter::InternalUpdate(vtkImageData *outData)
{
  // We could handle NULLs in our input list, but ...
  if ( ! this->Inputs || ! this->Inputs[0])
    {
    vtkErrorMacro("First input is not set.");
    return;
    }
  
  // prevent infinite update loops.
  if (this->Updating)
    {
    return;
    }
  this->Updating = 1;
  this->AbortExecute = 0;
  
  // Make sure there is an output.
  this->CheckCache();
  
  // In case this update is called directly.
  this->UpdateImageInformation();
  this->Output->ClipUpdateExtentWithWholeExtent();
  
  // Handle bypass condition.
  if (this->Bypass)
    {
    vtkImageData *inData;

    this->Inputs[0]->SetUpdateExtent(this->Output->GetUpdateExtent());
    inData = this->Inputs[0]->UpdateAndReturnData();
    if (!inData)
      {
      vtkWarningMacro("No input data provided!");
      }
    else
      {
      outData->GetPointData()->PassData(inData->GetPointData());
      }

    // release input data
    if (this->Inputs[0]->ShouldIReleaseData())
      {
      this->Inputs[0]->ReleaseData();
      }
    this->Updating = 0;
    return;
    }  
  
  this->RecursiveStreamUpdate(outData);

  this->Updating = 0;
}

//----------------------------------------------------------------------------
// This method can be called recursively for streaming.
// The extent of the outRegion changes, dim remains the same.
void vtkImageMultipleInputFilter::RecursiveStreamUpdate(vtkImageData *outData)
{
  int idx;
  int memory, divide;
  int outExt[6], splitExt[6];
  
  memory = 0;
  
  // Compute the required input region extent.
  // Copy to fill in extent of extra dimensions.
  divide = 0;
  for (idx = 0; idx < this->NumberOfInputs; idx++)
    {
    if (this->Inputs[idx])
      {
      this->ComputeRequiredInputUpdateExtent(
		     this->Inputs[idx]->GetUpdateExtent(),
		     this->Output->GetUpdateExtent(),idx);
      // determine the amount of memory that will be used by the input region.
      memory = this->Inputs[idx]->GetUpdateExtentMemorySize();
      if (memory > this->Inputs[idx]->GetMemoryLimit())
	{
	divide = 1;
	}
      }
    }
  
  // Split the inRegion if we are streaming.
  if (divide)
    {
    this->Output->GetUpdateExtent(outExt);
    if (this->SplitExtent(splitExt, outExt, 0, 2) > 1)
      { // yes we can split
      vtkDebugMacro(<< "RecursiveStreamUpdate: Splitting " 
                    << " : memory = " << memory);
      this->Output->SetUpdateExtent(splitExt);
      this->RecursiveStreamUpdate(outData);
      // Set the second half to update
      this->SplitExtent(splitExt, outExt, 1, 2);
      this->Output->SetUpdateExtent(splitExt);
      this->RecursiveStreamUpdate(outData);
      // Restore the original extent
      this->Output->SetUpdateExtent(outExt);
      return;
      }
    else
      {
      // Cannot split any more.  Ignore memory limit and continue.
      vtkWarningMacro(<< "RecursiveStreamUpdate: Cannot split. memory = "
        << memory << endl);
      }
    }

  // No Streaming required.
  vtkImageData **inDatas;
  inDatas = new vtkImageData *[this->NumberOfInputs];
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx])
      {
      // Get the input region (Update extent was set at start of this method).
      inDatas[idx] = this->Inputs[idx]->UpdateAndReturnData();
      }
    else
      {  
      // Input does not presently exist.
      inDatas[idx] = NULL;
      }  
    }

  // The StartMethod call is placed here to be after updating the input.
  if ( this->StartMethod )
    {
    (*this->StartMethod)(this->StartMethodArg);
    }
  // fill the output region 
  this->Execute(inDatas, outData);
  if ( this->EndMethod )
    {
    (*this->EndMethod)(this->EndMethodArg);
    }
  
  // Like the graphics pipeline this source releases inputs data.
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs && this->Inputs[idx] &&
	this->Inputs[idx]->ShouldIReleaseData())
      {
      this->Inputs[idx]->ReleaseData();
      }
    }
  
  delete [] inDatas;
}

//----------------------------------------------------------------------------
// This method gets the boundary of the inputs then computes and returns 
// the boundary of the largest region that can be generated. 
void vtkImageMultipleInputFilter::UpdateImageInformation()
{
  int idx;
  
  // Make sure the Input has been set.
  // we require that input 1 be set.
  if ( ! this->Inputs[0])
    {
    vtkErrorMacro(<< "UpdateImageInformation: Input is not set.");
    return;
    }
  
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx])
      {
      this->Inputs[idx]->UpdateImageInformation();
      }
    }
  
  // make sure we have an output
  this->CheckCache();
  
  // Set the defaults from input1
  this->Output->SetWholeExtent(this->Inputs[0]->GetWholeExtent());
  this->Output->SetSpacing(this->Inputs[0]->GetSpacing());
  this->Output->SetOrigin(this->Inputs[0]->GetOrigin());
  this->Output->SetScalarType(this->Inputs[0]->GetScalarType());
  this->Output->SetNumberOfScalarComponents(
			    this->Inputs[0]->GetNumberOfScalarComponents());
  if ( ! this->Bypass)
    {
    // Let the subclass modify the default.
    this->ExecuteImageInformation();
    }
}



//----------------------------------------------------------------------------
// This method is passed an inRegion that holds the image information
// (image extent ...) of this filters input, and fills outRegion with
// the image information after this filter is finished.
// outImage is identical to inImage when this method is envoked, and
// outImage may be the same object as in image.
void vtkImageMultipleInputFilter::ExecuteImageInformation()
{
  // Default: Image information does not change (do nothing).
}



//----------------------------------------------------------------------------
// This method computes the extent of the input region necessary to generate
// an output region.  Before this method is called "region" should have the 
// extent of the output region.  After this method finishes, "region" should 
// have the extent of the required input region.  The default method assumes
// the required input extent are the same as the output extent.
// Note: The splitting methods call this method with outRegion = inRegion.
void 
vtkImageMultipleInputFilter::ComputeRequiredInputUpdateExtent(int inExt[6],
							      int outExt[6],
							      int whichInput)
{
  whichInput = whichInput;
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

  













