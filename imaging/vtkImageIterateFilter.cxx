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
  // for filters that execute multiple times
  this->Iteration = 0;
  this->NumberOfIterations = 0;
  this->IterationCaches = NULL;
  this->SetNumberOfIterations(1);
}

//----------------------------------------------------------------------------
vtkImageIterateFilter::~vtkImageIterateFilter()
{
  this->SetNumberOfIterations(0);
}

//----------------------------------------------------------------------------
void vtkImageIterateFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageFilter::PrintSelf(os,indent);

  os << indent << "NumberOfIterations: " << this->NumberOfIterations << "\n";

  // This variable is included here to pass the PrintSelf test.
  // The variable is public to get around a compiler issue.
  // this->Iteration
}

  
//----------------------------------------------------------------------------
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
  if ( this->StartMethod )
    {
    (*this->StartMethod)(this->StartMethodArg);
    }
  // fill the output region 
  this->IterateExecute(inData, outData);
  if ( this->EndMethod )
    {
    (*this->EndMethod)(this->EndMethodArg);
    }
  
}


//----------------------------------------------------------------------------
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
    
    /* default value */
    memcpy(this->Input->GetUpdateExtent(), this->Output->GetUpdateExtent(),
	   6 * sizeof(int));
    
    this->ComputeRequiredInputUpdateExtent(this->Input->GetUpdateExtent(),
					   this->Output->GetUpdateExtent());
    }
  
  // restore the original input and output.
  this->Input = this->IterationCaches[0];
  this->Output = this->IterationCaches[this->NumberOfIterations];
}


//----------------------------------------------------------------------------
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
      this->IterationCaches[idx]->UnRegister(this);
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

