/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagePadFilter.cxx
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
#include "vtkImagePadFilter.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkImagePadFilter, "1.26");
vtkStandardNewMacro(vtkImagePadFilter);

//----------------------------------------------------------------------------
// Constructor sets default values
vtkImagePadFilter::vtkImagePadFilter()
{
  int idx;

  // Initialize output image extent to INVALID
  for (idx = 0; idx < 3; ++idx)
    {
    this->OutputWholeExtent[idx * 2] = 0;
    this->OutputWholeExtent[idx * 2 + 1] = -1;
    }
  // Set Output numberOfScalarComponents to INVALID
  this->OutputNumberOfScalarComponents = -1;
}

//----------------------------------------------------------------------------
void vtkImagePadFilter::SetOutputWholeExtent(int extent[6])
{
  int idx, modified = 0;
  
  for (idx = 0; idx < 6; ++idx)
    {
    if (this->OutputWholeExtent[idx] != extent[idx])
      {
      this->OutputWholeExtent[idx] = extent[idx];
      modified = 1;
      }
    }

  if (modified)
    {
    this->Modified();
    }
}
//----------------------------------------------------------------------------
void vtkImagePadFilter::SetOutputWholeExtent(int minX, int maxX, 
                                             int minY, int maxY,
                                             int minZ, int maxZ)
{
  int extent[6];
  
  extent[0] = minX;  extent[1] = maxX;
  extent[2] = minY;  extent[3] = maxY;
  extent[4] = minZ;  extent[5] = maxZ;
  this->SetOutputWholeExtent(extent);
}


//----------------------------------------------------------------------------
void vtkImagePadFilter::GetOutputWholeExtent(int extent[6])
{
  int idx;
  
  for (idx = 0; idx < 6; ++idx)
    {
    extent[idx] = this->OutputWholeExtent[idx];
    }
}


//----------------------------------------------------------------------------
// Just change the Image extent.
void vtkImagePadFilter::ExecuteInformation(vtkImageData *inData, 
                                           vtkImageData *outData)
{
  if (this->OutputWholeExtent[0] > this->OutputWholeExtent[1])
    {
    // invalid setting, it has not been set, so default to whole Extent
    inData->GetWholeExtent(this->OutputWholeExtent);
    }
  outData->SetWholeExtent(this->OutputWholeExtent);
  
  if (this->OutputNumberOfScalarComponents < 0)
    {
    // invalid setting, it has not been set, so default to input.
    this->OutputNumberOfScalarComponents 
      = inData->GetNumberOfScalarComponents();
    }
  outData->SetNumberOfScalarComponents(this->OutputNumberOfScalarComponents);
}

//----------------------------------------------------------------------------
// Just clip the request.  The subclass may need to overwrite this method.
void vtkImagePadFilter::ComputeInputUpdateExtent(int inExt[6], 
                                                 int outExt[6])
{
  int idx;
  int *wholeExtent;
  
  // handle XYZ
  memcpy(inExt,outExt,sizeof(int)*6);
  
  wholeExtent = this->GetInput()->GetWholeExtent();
  // Clip
  for (idx = 0; idx < 3; ++idx)
    {
    if (inExt[idx*2] < wholeExtent[idx*2])
      {
      inExt[idx*2] = wholeExtent[idx*2];
      }
    if (inExt[idx*2] > wholeExtent[idx*2 + 1])
      {
      inExt[idx*2] = wholeExtent[idx*2 + 1];
      }
    if (inExt[idx*2+1] < wholeExtent[idx*2])
      {
      inExt[idx*2+1] = wholeExtent[idx*2];
      }
    if (inExt[idx*2 + 1] > wholeExtent[idx*2 + 1])
      {
      inExt[idx*2 + 1] = wholeExtent[idx*2 + 1];
      }
    }
}

void vtkImagePadFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "OutputNumberOfScalarComponents: " 
     << this->OutputNumberOfScalarComponents << "\n";
}


