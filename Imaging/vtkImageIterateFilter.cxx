/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageIterateFilter.cxx
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
#include "vtkImageIterateFilter.h"

vtkCxxRevisionMacro(vtkImageIterateFilter, "1.31");

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
  this->Superclass::PrintSelf(os,indent);

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

void vtkImageIterateFilter::ComputeInputUpdateExtents( vtkDataObject *output )
{
  vtkImageData *in, *out = (vtkImageData*)output;
  int inExt[6], idx;

  if (!this->GetInput())
    {
    vtkErrorMacro(<< "Input not set.");
    return;
    }

  for (idx = this->NumberOfIterations - 1; idx >= 0; --idx)
    {
    this->Iteration = idx;
    in = this->GetIterationInput();
    
    if (!in)
      {
      return;
      }
    /* default value */
    out->GetUpdateExtent(inExt);
    this->ComputeInputUpdateExtent(inExt, out->GetUpdateExtent());
    in->SetUpdateExtent(inExt);
    out = in;
    }
}

//----------------------------------------------------------------------------
void vtkImageIterateFilter::AllocateOutputScalars(vtkImageData *outData)
{
  outData->SetExtent(outData->GetUpdateExtent());
  outData->AllocateScalars();
}

//----------------------------------------------------------------------------
// Some filters (decomposes, anisotropic difusion ...) have execute 
// called multiple times per update.
void vtkImageIterateFilter::ExecuteData(vtkDataObject *vtkNotUsed(out))
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
    
    this->AllocateOutputScalars(outData);

    // execute for this iteration
    this->IterativeExecuteData(inData, outData);
    
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

    // Let the subclass modify the default.
    this->ExecuteInformation(in, out);
    }
}

//----------------------------------------------------------------------------
//  Called by the above for each decomposition.  Subclass can modify
// the defaults by implementing this method.
void vtkImageIterateFilter::ExecuteInformation(vtkImageData *vtkNotUsed(inData),
                                       vtkImageData *vtkNotUsed(outData))
{
}


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









