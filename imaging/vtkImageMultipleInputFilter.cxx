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
}

//----------------------------------------------------------------------------
void vtkImageMultipleInputFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "NumberOfThreads: " << this->NumberOfThreads << "\n";
  os << indent << "Bypass: " << this->Bypass << "\n";
  
  vtkImageSource::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
unsigned long int vtkImageMultipleInputFilter::GetPipelineMTime()
{
  unsigned long int time1, time2;
  int idx;

  // This objects MTime
  // (Super class considers cache in case cache did not originate message)
  time1 = this->GetMTime();
  // Look at input modified times.
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if ( ! this->Inputs[idx])
      {
      time2 = time1;
      }
    else
      {
      this->Inputs[idx]->UpdateInformation();
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
void vtkImageMultipleInputFilter::AddInput(vtkImageData *input)
{
  this->vtkProcessObject::AddInput(input);
}

//----------------------------------------------------------------------------
// Set an Input of this filter. 
void vtkImageMultipleInputFilter::SetInput(int idx, vtkImageData *input)
{
  this->vtkProcessObject::SetInput(idx, input);
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
// Called by cache
void vtkImageMultipleInputFilter::InternalUpdate(vtkDataObject *data)
{
  vtkImageData *outData = (vtkImageData *)(data);
    
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
  
  // In case this update is called directly.
  this->UpdateInformation();
  this->GetOutput()->ClipUpdateExtentWithWholeExtent();
  
  // since cache no longer exists we must allocate the scalars here
  // This may be a bad place to allocate data (before input->update)
  this->InterceptCacheUpdate();
  outData->SetExtent(outData->GetUpdateExtent());
  outData->AllocateScalars();  
  
  // Handle bypass condition.
  if (this->Bypass)
    {
    vtkImageData *inData;

    this->GetInput(0)->SetUpdateExtent(this->GetOutput()->GetUpdateExtent());
    this->GetInput(0)->Update();
    inData = this->GetInput(0);
    if (!inData)
      {
      vtkWarningMacro("No input data provided!");
      }
    else
      {
      outData->GetPointData()->PassData(inData->GetPointData());
      }

    // release input data
    if (this->GetInput(0)->ShouldIReleaseData())
      {
      this->GetInput(0)->ReleaseData();
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
  int inExt[6], outExt[6], splitExt[6];
  
  memory = 0;
  
  // Compute the required input region extent.
  // Copy to fill in extent of extra dimensions.
  divide = 0;
  for (idx = 0; idx < this->NumberOfInputs; idx++)
    {
    if (this->Inputs[idx])
      {
      this->ComputeInputUpdateExtent(inExt,
			 this->GetOutput()->GetUpdateExtent(),idx);
      this->GetInput(idx)->SetUpdateExtent(inExt),
      // determine the amount of memory that will be used by the input region.
      memory = this->GetInput(idx)->GetUpdateExtentMemorySize();
      if (memory > this->GetInput(idx)->GetMemoryLimit())
	{
	divide = 1;
	}
      }
    }
  
  // Split the inRegion if we are streaming.
  if (divide)
    {
    this->GetOutput()->GetUpdateExtent(outExt);
    if (this->SplitExtent(splitExt, outExt, 0, 2) > 1)
      { // yes we can split
      vtkDebugMacro(<< "RecursiveStreamUpdate: Splitting " 
                    << " : memory = " << memory);
      this->GetOutput()->SetUpdateExtent(splitExt);
      this->RecursiveStreamUpdate(outData);
      // Set the second half to update
      this->SplitExtent(splitExt, outExt, 1, 2);
      this->GetOutput()->SetUpdateExtent(splitExt);
      this->RecursiveStreamUpdate(outData);
      // Restore the original extent
      this->GetOutput()->SetUpdateExtent(outExt);
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
      this->GetInput(idx)->InternalUpdate();
      inDatas[idx] = this->GetInput(idx);
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
void vtkImageMultipleInputFilter::UpdateInformation()
{
  int idx;
  
  // Make sure the Input has been set.
  // we require that input 1 be set.
  if ( ! this->Inputs[0])
    {
    vtkErrorMacro(<< "UpdateInformation: Input is not set.");
    return;
    }
  
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx])
      {
      this->Inputs[idx]->UpdateInformation();
      }
    }
  
  // Set the defaults from input1
  this->GetOutput()->SetWholeExtent(this->GetInput(0)->GetWholeExtent());
  this->GetOutput()->SetSpacing(this->GetInput(0)->GetSpacing());
  this->GetOutput()->SetOrigin(this->GetInput(0)->GetOrigin());
  this->GetOutput()->SetScalarType(this->GetInput(0)->GetScalarType());
  this->GetOutput()->SetNumberOfScalarComponents(
			    this->GetInput(0)->GetNumberOfScalarComponents());
  if ( ! this->Bypass)
    {
    // Let the subclass modify the default.
    this->ExecuteInformation();
    }
}



//----------------------------------------------------------------------------
// This method is passed an inRegion that holds the image information
// (image extent ...) of this filters input, and fills outRegion with
// the image information after this filter is finished.
// outImage is identical to inImage when this method is envoked, and
// outImage may be the same object as in image.
void vtkImageMultipleInputFilter::ExecuteInformation()
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
void vtkImageMultipleInputFilter::ComputeInputUpdateExtent(int inExt[6],
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













