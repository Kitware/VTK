/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageIterateFilter.cxx
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
#include "vtkImageSimpleCache.h"
#include "vtkImageIterateFilter.h"

//----------------------------------------------------------------------------
vtkImageIterateFilter::vtkImageIterateFilter()
{
  this->Input = NULL;
  this->Bypass = 0;
  this->Updating = 0;
  this->Threader = vtkMultiThreader::New();
  this->NumberOfThreads = this->Threader->GetNumberOfThreads();
  // for filters that execute multiple times
  this->Iteration = 0;
  this->NumberOfIterations = 0;
  this->IterationCaches = NULL;
  this->SetNumberOfIterations(1);
}

//----------------------------------------------------------------------------
vtkImageIterateFilter::~vtkImageIterateFilter()
{
  this->Threader->Delete();
  this->SetNumberOfIterations(0);
}

//----------------------------------------------------------------------------
void vtkImageIterateFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Bypass: " << this->Bypass << "\n";
  os << indent << "Input: (" << this->Input << ").\n";
  os << indent << "NumberOfThreads: " << this->NumberOfThreads << "\n";

  vtkImageSource::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
// Description:
// This Method returns the MTime of the pipeline upto and including this filter
// Note: current implementation may create a cascade of GetPipelineMTime calls.
// Each GetPipelineMTime call propagates the call all the way to the original
// source.  
unsigned long int vtkImageIterateFilter::GetPipelineMTime()
{
  unsigned long int time1, time2;

  // This objects MTime
  // (Super class considers cache in case cache did not originate message)
  time1 = this->vtkImageSource::GetPipelineMTime();
  if ( ! this->Input)
    {
    vtkWarningMacro(<< "GetPipelineMTime: Input not set.");
    return time1;
    }
  
  // Pipeline mtime 
  time2 = this->Input->GetPipelineMTime();
  
  // Return the larger of the two 
  if (time2 > time1)
    time1 = time2;

  return time1;
}


//----------------------------------------------------------------------------
// Description:
// Set the Input of a filter. 
void vtkImageIterateFilter::SetInput(vtkImageCache *input)
{
  vtkDebugMacro(<< "SetInput: input = " << input->GetClassName()
		<< " (" << input << ")");

  // does this change anything?
  if (input == this->Input)
    {
    return;
    }
  
  this->Input = input;
  this->Modified();
}


//----------------------------------------------------------------------------
// Description:
// This method is called by the cache.  It eventually calls the
// Execute(vtkImageData *, vtkImageData *) method.
// ImageInformation has already been updated by this point, 
// and outRegion is in local coordinates.
// This method will stream to get the input, and loops over extra axes.
// Only the UpdateExtent from output will get updated.
void vtkImageIterateFilter::InternalUpdate(vtkImageData *outData)
{
  // Make sure the Input has been set.
  if ( ! this->Input)
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
  
  // Handle bypass condition.
  if (this->Bypass)
    {
    vtkImageData *inData;

    this->Input->SetUpdateExtent(this->Output->GetUpdateExtent());
    inData = this->Input->UpdateAndReturnData();
    if (!inData)
      {
      vtkWarningMacro("No input data provided!");
      }
    else
      {
      outData->GetPointData()->PassData(inData->GetPointData());
      }

    // release input data
    if (this->Input->ShouldIReleaseData())
      {
      this->Input->ReleaseData();
      }
    this->Updating = 0;
    return;
    }
  
  this->RecursiveStreamUpdate(outData);

  this->Updating = 0;
}



  
//----------------------------------------------------------------------------
// Description:
// This method can be called recursively for streaming.
// The extent of the outRegion changes, dim remains the same.
void vtkImageIterateFilter::RecursiveStreamUpdate(vtkImageData *outData)
{
  int memory;
  vtkImageData *inData;
  int outExt[6], splitExt[6];
  
  // Compute the required input region extent.
  // Copy to fill in extent of extra dimensions.
  this->IterateRequiredInputUpdateExtent();
    
  // determine the amount of memory that will be used by the input region.
  memory = this->Input->GetUpdateExtentMemorySize();
  
  // Split the inRegion if we are streaming.
  if ((memory > this->Input->GetMemoryLimit()))
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
                      << memory);
      }
    }

  // No Streaming required.
  // Get the input region (Update extent was set at start of this method).
  inData = this->Input->UpdateAndReturnData();

  // The StartMethod call is placed here to be after updating the input.
  if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
  // fill the output region 
  this->IterateExecute(inData, outData);
  if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
  
}


//----------------------------------------------------------------------------
// Description:
// For streaming and threads.  Splits output update extent into num pieces.
// This method needs to be called num times.  Results must not overlap for
// consistent starting extent.  Subclass can override this method.
// This method returns the number of peices resulting from a successful split.
// This can be from 1 to "total".  
// If 1 is returned, the extent cannot be split.
int vtkImageIterateFilter::SplitExtent(int splitExt[6], int startExt[6], 
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
  if ((max - min + 1) < total)
    {
    total = max - min + 1;
    }
  
  if (num >= total)
    {
    vtkDebugMacro("  SplitRequest (" << num 
		  << ") larger than total: " << total);
    return total;
    }
  
  // determine the extent of the piece
  splitExt[splitAxis*2] = min + (max - min + 1)*num/total;
  if (num == total - 1)
    {
    splitExt[splitAxis*2+1] = max;
    }
  else
    {
    splitExt[splitAxis*2+1] = (min-1) + (max - min + 1)*(num+1)/total;
    }
  
  vtkDebugMacro("  Split Piece: ( " <<splitExt[0]<< ", " <<splitExt[1]<< ", "
		<< splitExt[2] << ", " << splitExt[3] << ", "
		<< splitExt[4] << ", " << splitExt[5] << ")");
  fflush(stderr);

  return total;  
}

  
  
//----------------------------------------------------------------------------
// Description:
// Some filters (decomposes, anisotropic difusion ...) have execute 
// called multiple times per update.
void vtkImageIterateFilter::IterateExecute(vtkImageData *inData, 
					   vtkImageData *outData)
{
  int idx;

  // IterateCaches are all set up 
  // see: SetNumberOfIterations() and UpdateImageInformation()
  for (idx = 0; idx < this->NumberOfIterations; ++idx)
    {
    this->Iteration = idx;
    // temporarily change input and output
    this->Input = this->IterationCaches[idx];
    this->Output = this->IterationCaches[idx + 1];
    // Get the data
    inData = this->Input->GetData();
    outData = this->Output->GetData();
    // execute for this iteration
    this->Execute(inData, outData);
    // Like the graphics pipeline this source releases inputs data.
    if (this->Input->ShouldIReleaseData())
      {
      this->Input->ReleaseData();
      }
    }
  // restore the original input and output.
  this->Input = this->IterationCaches[0];
  this->Output = this->IterationCaches[this->NumberOfIterations];
}




//----------------------------------------------------------------------------
// Description:
// This method sets the WholeExtent, Spacing and Origin of the output.
void vtkImageIterateFilter::UpdateImageInformation()
{
  vtkImageCache *in, *out;
  int idx;
  
  // Make sure the Input has been set.
  if ( ! this->Input)
    {
    vtkErrorMacro(<< "UpdateImageInformation: Input is not set.");
    return;
    }
  // make sure we have an output
  this->CheckCache();

  // put the input and output into the cache list.
  // Iteration caches
  this->IterationCaches[0] = this->Input;
  this->IterationCaches[this->NumberOfIterations] = this->Output;
  
  this->Input->UpdateImageInformation();
  for (idx = 0; idx < this->NumberOfIterations; ++idx)
    {
    this->Iteration = idx;
    in = this->IterationCaches[idx];
    out = this->IterationCaches[idx+1];
    
    // temporarily overide the input and output (see comments in .h)
    // passed as parameters?
    this->Input = in;
    this->Output = out;
    
    // Set up the defaults
    out->SetWholeExtent(in->GetWholeExtent());
    out->SetSpacing(in->GetSpacing());
    out->SetOrigin(in->GetOrigin());
    out->SetScalarType(in->GetScalarType());
    out->SetNumberOfScalarComponents(in->GetNumberOfScalarComponents());

    if ( ! this->Bypass)
      {
      // Let the subclass modify the default.
      this->ExecuteImageInformation();
      }
    }

  // restore the original input and output.
  this->Input = this->IterationCaches[0];
  this->Output = this->IterationCaches[this->NumberOfIterations];
}



//----------------------------------------------------------------------------
// Description:
// This method can be overriden in a subclass to compute the output
// ImageInformation: WholeExtent, Spacing and Origin.
void vtkImageIterateFilter::ExecuteImageInformation()
{
}


//----------------------------------------------------------------------------
// Description:
// This method can be overriden in a subclass to compute the input
// UpdateExtent needed to generate the output UpdateExtent.
// By default the input is set to the same as the output before this
// method is called.
void vtkImageIterateFilter::IterateRequiredInputUpdateExtent()
{
  int idx;
  
  for (idx = this->NumberOfIterations - 1; idx >= 0; --idx)
    {
    this->Iteration = idx;
    // Set up tempoary input and output
    this->Input = this->IterationCaches[idx];
    this->Output = this->IterationCaches[idx+1];
    
    this->ComputeRequiredInputUpdateExtent(this->Input->GetUpdateExtent(),
					   this->Output->GetUpdateExtent());
    }
  
  // restore the original input and output.
  this->Input = this->IterationCaches[0];
  this->Output = this->IterationCaches[this->NumberOfIterations];
}


//----------------------------------------------------------------------------
// Description:
// This method can be overriden in a subclass to compute the input
// UpdateExtent needed to generate the output UpdateExtent.
// By default the input is set to the same as the output before this
// method is called.
void vtkImageIterateFilter::ComputeRequiredInputUpdateExtent(int inExt[6], int outExt[6])
{
  memcpy(inExt,outExt,sizeof(int)*6);
}

struct vtkImageThreadStruct
{
  vtkImageIterateFilter *Filter;
  vtkImageData   *Input;
  vtkImageData   *Output;
};



// this mess is really a simple function. All it does is call
// the ThreadedExecute method after setting the correct
// extent for this thread. Its just a pain to calculate
// the correct extent.
VTK_THREAD_RETURN_TYPE vtkImageIterateThreadedExecute( void *arg )
{
  vtkImageThreadStruct *str;
  int ext[6], splitExt[6], total;
  int threadId, threadCount;
  
  threadId = ((ThreadInfoStruct *)(arg))->ThreadID;
  threadCount = ((ThreadInfoStruct *)(arg))->NumberOfThreads;

  str = (vtkImageThreadStruct *)(((ThreadInfoStruct *)(arg))->UserData);
  
  memcpy(ext,str->Filter->GetOutput()->GetUpdateExtent(),
	 sizeof(int)*6);

  // execute the actual method with appropriate extent
  // first find out how many pieces extent can be split into.
  total = str->Filter->SplitExtent(splitExt, ext, threadId, threadCount);
    
  if (threadId < total)
    {
    str->Filter->ThreadedExecute(str->Input, str->Output, splitExt, threadId);
    }
  else
    {
    // otherwise don't use this thread. Sometimes the threads dont
    // break up very well and it is just as efficient to leave a 
    // few threads idle.
    }
  
  return VTK_THREAD_RETURN_VALUE;
}

void vtkImageIterateFilter::Execute(vtkImageData *inData, 
			     vtkImageData *outData)
{
  vtkImageThreadStruct str;
  
  str.Filter = this;
  str.Input = inData;
  str.Output = outData;
  
  this->Threader->SetNumberOfThreads(this->NumberOfThreads);
  
  // setup threading and the invoke threadedExecute
  this->Threader->SetSingleMethod(vtkImageIterateThreadedExecute, &str);
  this->Threader->SingleMethodExecute();
}


//----------------------------------------------------------------------------
// Description:
// The execute method created by the subclass.
void vtkImageIterateFilter::ThreadedExecute(vtkImageData *vtkNotUsed(inData), 
				     vtkImageData *vtkNotUsed(outData),
				     int extent[6], int threadId)
{
  extent = extent;
  threadId = threadId;
  
  vtkErrorMacro("subclase should override this method!!!");
}



//----------------------------------------------------------------------------
// Description:
// Filters that execute multiple times per update use this internal method.
void vtkImageIterateFilter::SetNumberOfIterations(int num)
{
  int idx;
  
  if (num == this->NumberOfIterations)
    {
    return;
    }
  
  // delete previous temporary caches 
  // (first and last are global input and output)
  if (this->IterationCaches)
    {
    for (idx = 1; idx < this->NumberOfIterations; ++idx)
      {
      this->IterationCaches[idx]->Delete();
      this->IterationCaches[idx] = NULL;
      }
    delete this->IterationCaches;
    this->IterationCaches = NULL;
    }

  // special case for destructor
  if (num == 0)
    {
    return;
    }
  
  // create new ones (first and last set later to input and output)
  this->IterationCaches = (vtkImageCache **) new void *[num + 1];
  this->IterationCaches[0] = this->IterationCaches[num] = NULL;
  for (idx = 1; idx < num; ++idx)
    {
    this->IterationCaches[idx] = vtkImageSimpleCache::New();
    this->IterationCaches[idx]->ReleaseDataFlagOn();
    }

  this->NumberOfIterations = num;
  this->Modified();
}

