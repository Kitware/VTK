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
  vtkImageToImageFilter::PrintSelf(os,indent);

  os << indent << "NumberOfIterations: " << this->NumberOfIterations << "\n";

  // This variable is included here to pass the PrintSelf test.
  // The variable is public to get around a compiler issue.
  // this->Iteration
}


//----------------------------------------------------------------------------
vtkImageData *vtkImageIterateFilter::GetIterationInput()
{
  if (this->IterationCaches == NULL || this->Iteration == 0)
    {
    // error, or return input ???
    return this->GetInput();
    }
  return this->IterationCaches[this->Iteration];
}


//----------------------------------------------------------------------------
vtkImageData *vtkImageIterateFilter::GetIterationOutput()
{
  if (this->IterationCaches == NULL || 
      this->Iteration == this->NumberOfIterations-1)
    {
    // error, or return output ???
    return this->GetOutput();
    }
  return this->IterationCaches[this->Iteration+1];
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
  memory = this->GetInput()->GetUpdateExtentMemorySize();
  
  // Split the inRegion if we are streaming.
  if ((memory > this->GetInput()->GetMemoryLimit()))
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
                      << memory);
      }
    }

  // No Streaming required.
  // Get the input region (Update extent was set at start of this method).
  this->GetInput()->Update();
  inData = this->GetInput();

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
  // see: SetNumberOfIterations() and UpdateInformation()
  for (idx = 0; idx < this->NumberOfIterations; ++idx)
    {
    this->Iteration = idx;
    // temporarily change input and output
    inData = this->IterationCaches[idx];
    outData = this->IterationCaches[idx + 1];
    
    // since cache no longer exists we must allocate the scalars here
    // This may be a bad place to allocate data (before input->update)
    outData->SetExtent(outData->GetUpdateExtent());
    outData->AllocateScalars();      
    
    // execute for this iteration
    this->Execute(inData, outData);
    // Like the graphics pipeline this source releases inputs data.
    if (inData->ShouldIReleaseData())
      {
      inData->ReleaseData();
      }
    }
}




//----------------------------------------------------------------------------
// This method sets the WholeExtent, Spacing and Origin of the output.
void vtkImageIterateFilter::UpdateInformation()
{
  vtkImageData *in, *out;
  int idx;
  
  // Make sure the Input has been set.
  if ( ! this->GetInput())
    {
    vtkErrorMacro(<< "UpdateInformation: Input is not set.");
    return;
    }

  // put the input and output into the cache list.
  // Iteration caches
  this->IterationCaches[0] = this->GetInput();
  this->IterationCaches[this->NumberOfIterations] = this->GetOutput();
  
  this->GetInput()->UpdateInformation();
  for (idx = 0; idx < this->NumberOfIterations; ++idx)
    {
    this->Iteration = idx;
    in = this->GetIterationInput();
    out = this->GetIterationOutput();
    
    // Set up the defaults
    out->SetWholeExtent(in->GetWholeExtent());
    out->SetSpacing(in->GetSpacing());
    out->SetOrigin(in->GetOrigin());
    out->SetScalarType(in->GetScalarType());
    out->SetNumberOfScalarComponents(in->GetNumberOfScalarComponents());

    if ( ! this->Bypass)
      {
      // Let the subclass modify the default.
      this->ExecuteInformation(in, out);
      }
    }

}

//----------------------------------------------------------------------------
//  Called by the above for each decomposition
void vtkImageIterateFilter::ExecuteInformation(vtkImageData *inData,
					       vtkImageData *outData)
{
}


//----------------------------------------------------------------------------
// This method can be overriden in a subclass to compute the input
// UpdateExtent needed to generate the output UpdateExtent.
// By default the input is set to the same as the output before this
// method is called.
void vtkImageIterateFilter::IterateRequiredInputUpdateExtent()
{
  vtkImageData *in, *out;
  int inExt[6], idx;
  
  for (idx = this->NumberOfIterations - 1; idx >= 0; --idx)
    {
    this->Iteration = idx;
    in = this->GetIterationInput();
    out = this->GetIterationOutput();
    
    /* default value */
    out->GetUpdateExtent(inExt);
    this->ComputeInputUpdateExtent(inExt, out->GetUpdateExtent());
    in->SetUpdateExtent(inExt);
    }
  
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
    delete [] this->IterationCaches;
    this->IterationCaches = NULL;
    }

  // special case for destructor
  if (num == 0)
    {
    return;
    }
  
  // create new ones (first and last set later to input and output)
  this->IterationCaches = (vtkImageData **) new void *[num + 1];
  this->IterationCaches[0] = this->IterationCaches[num] = NULL;
  for (idx = 1; idx < num; ++idx)
    {
    this->IterationCaches[idx] = vtkImageData::New();
    this->IterationCaches[idx]->ReleaseDataFlagOn();
    }

  this->NumberOfIterations = num;
  this->Modified();
}

