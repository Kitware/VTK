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
  this->IterationData = NULL;
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
  if (this->IterationData == NULL || this->Iteration == 0)
    {
    // error, or return input ???
    return this->GetInput();
    }
  return this->IterationData[this->Iteration];
}


//----------------------------------------------------------------------------
vtkImageData *vtkImageIterateFilter::GetIterationOutput()
{
  if (this->IterationData == NULL || 
      this->Iteration == this->NumberOfIterations-1)
    {
    // error, or return output ???
    return this->GetOutput();
    }
  return this->IterationData[this->Iteration+1];
}

  
//----------------------------------------------------------------------------
// Some filters (decomposes, anisotropic difusion ...) have execute 
// called multiple times per update.
void vtkImageIterateFilter::Execute()
{
  int idx;
  vtkImageData *inData, *outData;

  // IterationData are all set up 
  // see: SetNumberOfIterations() and UpdateInformation()
  for (idx = 0; idx < this->NumberOfIterations; ++idx)
    {
    this->Iteration = idx;
    // temporarily change input and output
    inData = this->IterationData[idx];
    outData = this->IterationData[idx + 1];
    
    // We have to allocate the IterationData somewhere.
    // Last (real) output already allocated. (Streaming ...)
    if (idx + 1 < this->NumberOfIterations)
      {
      outData->SetExtent(outData->GetUpdateExtent());
      outData->AllocateScalars();      
      }
    // execute for this iteration
    outData->GetUpdateExtent(this->ExecuteExtent);
    this->Execute(inData, outData);
    
    // Part of me thinks we should always release the 
    // intermediate (iteration) data.  But saving it could speed execution ...
    // Like the graphics pipeline this source releases inputs data.
    if (inData->ShouldIReleaseData())
      {
      inData->ReleaseData();
      }
    }
}


//----------------------------------------------------------------------------
// Hidden by Execute().
void vtkImageIterateFilter::Execute(vtkImageData *inData, vtkImageData *outData)
{
  this->vtkImageToImageFilter::Execute(inData, outData);
}




//----------------------------------------------------------------------------
// This method sets the WholeExtent, Spacing and Origin of the output.
void vtkImageIterateFilter::ExecuteInformation()
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
  this->IterationData[0] = this->GetInput();
  this->IterationData[this->NumberOfIterations] = this->GetOutput();
  
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
      this->ExecuteImageInformation(in, out);
      }
    }
}

//----------------------------------------------------------------------------
//  Called by the above for each decomposition.  Subclass can modify
// the defaults by implementing this method.
void vtkImageIterateFilter::ExecuteImageInformation(vtkImageData *inData,
						    vtkImageData *outData)
{
}


//----------------------------------------------------------------------------
int vtkImageIterateFilter::ComputeDivisionExtents(vtkDataObject *output,
						  int division, int numDivisions)
{
  vtkImageData *in, *out = (vtkImageData*)output;
  int inExt[6], *outExt, idx;
  
  // For now, lets disable streaming on iteration filters.  To be truly 
  // streaming (FFT) each iteration needs to break up it processing.  
  // Since this superclass no longer has an Update method 
  // (streming initiated in Execute) this is not possible.
  if (division != 0)
    {
    return 0;
    }
  
  // Since we are not interleaved with Execute calls,
  // we need to set ExecuteExtent somewhere else.
  // ...
  
  // well even though we only support one output, 
  // use the output passed in anyway.
  for (idx = this->NumberOfIterations - 1; idx >= 0; --idx)
    {
    this->Iteration = idx;
    in = this->GetIterationInput();
    
    /* default value */
    out->GetUpdateExtent(inExt);
    this->ComputeRequiredInputUpdateExtent(inExt, out->GetUpdateExtent());
    in->SetUpdateExtent(inExt);
    out = in;
    }
  return 1;
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
  if (this->IterationData)
    {
    for (idx = 1; idx < this->NumberOfIterations; ++idx)
      {
      this->IterationData[idx]->UnRegister(this);
      this->IterationData[idx] = NULL;
      }
    delete [] this->IterationData;
    this->IterationData = NULL;
    }

  // special case for destructor
  if (num == 0)
    {
    return;
    }
  
  // create new ones (first and last set later to input and output)
  this->IterationData = (vtkImageData **) new void *[num + 1];
  this->IterationData[0] = this->IterationData[num] = NULL;
  for (idx = 1; idx < num; ++idx)
    {
    this->IterationData[idx] = vtkImageData::New();
    this->IterationData[idx]->ReleaseDataFlagOn();
    }

  this->NumberOfIterations = num;
  this->Modified();
}









