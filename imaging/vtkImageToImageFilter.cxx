/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToImageFilter.cxx
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
#include "vtkImageToImageFilter.h"

//----------------------------------------------------------------------------
vtkImageToImageFilter::vtkImageToImageFilter()
{
  this->Bypass = 0;
  this->Updating = 0;
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
  os << indent << "Bypass: " << this->Bypass << "\n";
  os << indent << "NumberOfThreads: " << this->NumberOfThreads << "\n";

  vtkImageSource::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkImageToImageFilter::SetInput(vtkImageData *input)
{
  this->vtkProcessObject::SetInput(0, input);
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
// This Method returns the MTime of the pipeline upto and including this filter
// Note: current implementation may create a cascade of GetPipelineMTime calls.
// Each GetPipelineMTime call propagates the call all the way to the original
// source.  
unsigned long int vtkImageToImageFilter::GetPipelineMTime()
{
  unsigned long int time1, time2;

  // This objects MTime
  // (Super class considers cache in case cache did not originate message)
  time1 = this->GetMTime();
  if ( ! this->GetInput())
    {
    vtkWarningMacro(<< "GetPipelineMTime: Input not set.");
    return time1;
    }
  
  // Pipeline mtime 
  time2 = this->GetInput()->GetPipelineMTime();
  
  // Return the larger of the two 
  if (time2 > time1)
    {
    time1 = time2;
    }

  return time1;
}


//----------------------------------------------------------------------------
// This method is called by the cache.  It eventually calls the
// Execute(vtkImageData *, vtkImageData *) method.
// Information has already been updated by this point, 
// and outRegion is in local coordinates.
// This method will stream to get the input, and loops over extra axes.
// Only the UpdateExtent from output will get updated.
void vtkImageToImageFilter::InternalUpdate(vtkDataObject *data)
{
  vtkImageData *outData = (vtkImageData *)data;

  // Make sure the Input has been set.
  if ( ! this->GetInput())
    {
    vtkErrorMacro(<< "Input is not set.");
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

    this->GetInput()->SetUpdateExtent(this->GetOutput()->GetUpdateExtent());
    this->GetInput()->InternalUpdate();
    inData = this->GetInput();
    if (!inData)
      {
      vtkWarningMacro("No input data provided!");
      }
    else
      {
      outData->GetPointData()->PassData(inData->GetPointData());
      }

    // release input data
    if (this->GetInput()->ShouldIReleaseData())
      {
      this->GetInput()->ReleaseData();
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
void vtkImageToImageFilter::RecursiveStreamUpdate(vtkImageData *outData)
{
  int memory;
  vtkImageData *inData;
  int inExt[6], outExt[6];
    
  // abort if required
  if (this->AbortExecute) 
    {
    return;
    }
  
  // Compute the required input region extent.
  // Copy to fill in extent of extra dimensions.
  this->ComputeInputUpdateExtent(inExt, this->GetOutput()->GetUpdateExtent());
  this->GetInput()->SetUpdateExtent(inExt),    
  
  // determine the amount of memory that will be used by the input region.
  memory = this->GetInput()->GetUpdateExtentMemorySize();
  
  // Split the inRegion if we are streaming.
  if ((memory > this->GetInput()->GetMemoryLimit()))
    {
    this->GetOutput()->GetUpdateExtent(outExt);
    vtkWarningMacro("RecursiveStreamUpdate: Streaming disabled ")
    }
  
  // No Streaming required.
  // Get the input region (Update extent was set at start of this method).
  this->GetInput()->InternalUpdate();
  inData = this->GetInput();

  // The StartMethod call is placed here to be after updating the input.
  if ( this->StartMethod )
    {
    (*this->StartMethod)(this->StartMethodArg);
    }
  // fill the output region 
  this->Execute(inData, outData);
  if ( this->EndMethod )
    {
    (*this->EndMethod)(this->EndMethodArg);
    }
  
  // Like the graphics pipeline this source releases inputs data.
  if (this->GetInput()->ShouldIReleaseData())
    {
    this->GetInput()->ReleaseData();
    }
}


//----------------------------------------------------------------------------
// This method sets the WholeExtent, Spacing and Origin of the output.
void vtkImageToImageFilter::UpdateInformation()
{
  vtkImageData *input = this->GetInput();
  vtkImageData *output = this->GetOutput();
  int *iTmp;
  
  // Make sure the Input has been set.
  if ( ! input)
    {
    vtkErrorMacro(<< "UpdateInformation: Input is not set.");
    return;
    }

  this->vtkSource::UpdateInformation();

  // I do not like setting up the defaults here because it could modify
  // the output which would cause more Executes than necessary.
  // Try to detect an already initialized output.  This in no good either.
  // The input could change.  The real solution is to put defaults into.
  // ExecuteInformation.  Anyone who implements an ExecuteInformation
  // method would have to fill in all fields.
  iTmp = output->GetWholeExtent();
  if (iTmp[0] == 0 && iTmp[1] == 0 && iTmp[2] == 0 && 
      iTmp[3] == 0 && iTmp[4] == 0 && iTmp[5] == 0)
    {
    output->SetWholeExtent(input->GetWholeExtent());
	output->SetSpacing(input->GetSpacing());
	output->SetOrigin(input->GetOrigin());
    }

  if (output->GetScalarType() == VTK_VOID)
    {
    output->SetScalarType(input->GetScalarType());
    }
  if (output->GetNumberOfScalarComponents() == 0)
    {
    output->SetNumberOfScalarComponents(input->GetNumberOfScalarComponents());
    }

  if ( ! this->Bypass)
    {
    // legacy (hack to check to see if subclass used wrong method.
    this->ExecuteImageInformationHack = 1;
    this->ExecuteImageInformation();
    if (this->ExecuteImageInformationHack == 1)
      {
      vtkWarningMacro("Rename your ExecuteImageInformation to ExecuteInformation");
      }
    // the correct call.
    // Let the subclass modify the default.
    this->ExecuteInformation();
    }
}


//----------------------------------------------------------------------------
// This method can be overriden in a subclass to compute the output
// Information: WholeExtent, Spacing and Origin.
void vtkImageToImageFilter::ExecuteInformation()
{
}

//----------------------------------------------------------------------------
// This method can be overriden in a subclass to compute the input
// UpdateExtent needed to generate the output UpdateExtent.
// By default the input is set to the same as the output before this
// method is called.
void vtkImageToImageFilter::ComputeInputUpdateExtent(int inExt[6], int outExt[6])
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
  
  threadId = ((ThreadInfoStruct *)(arg))->ThreadID;
  threadCount = ((ThreadInfoStruct *)(arg))->NumberOfThreads;

  str = (vtkImageThreadStruct *)(((ThreadInfoStruct *)(arg))->UserData);
  memcpy(ext,str->Output->GetUpdateExtent(),
	 sizeof(int)*6);

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


void vtkImageToImageFilter::Execute(vtkImageData *inData, 
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
				     int extent[6], int vtkNotUsed(threadId))
{
  extent = extent;
  vtkErrorMacro("subclass should override this method!!!");
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





//----------------------------------------------------------------------------
void vtkImageToImageFilter::SetInputMemoryLimit(int limit)
{
  vtkImageData *input = this->GetInput();
  
  if ( input == NULL)
    {
    vtkErrorMacro("Input must be set before InputMemoryLimit.");
    }
  
  input->SetMemoryLimit(limit);
}

//----------------------------------------------------------------------------
long vtkImageToImageFilter::GetInputMemoryLimit()
{
  vtkImageData *input = this->GetInput();
  
  if ( input == NULL)
    {
    vtkErrorMacro("Input must be set before you can get InputMemoryLimit.");
    return 1000000;
    }
  
  return input->GetMemoryLimit();
}
