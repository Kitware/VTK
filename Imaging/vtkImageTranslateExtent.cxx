/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageTranslateExtent.cxx
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
#include "vtkImageTranslateExtent.h"
#include "vtkObjectFactory.h"


vtkCxxRevisionMacro(vtkImageTranslateExtent, "1.18");
vtkStandardNewMacro(vtkImageTranslateExtent);

//----------------------------------------------------------------------------
vtkImageTranslateExtent::vtkImageTranslateExtent()
{
  int idx;

  for (idx = 0; idx < 3; ++idx)
    {
    this->Translation[idx]  = 0;
    }
}


//----------------------------------------------------------------------------
void vtkImageTranslateExtent::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Translation: (" << this->Translation[0]
     << "," << this->Translation[1] << "," << this->Translation[2] << endl;
}
  



//----------------------------------------------------------------------------
// Change the WholeExtent
void vtkImageTranslateExtent::ExecuteInformation(vtkImageData *inData, 
                                                 vtkImageData *outData)
{
  int idx, extent[6];
  float *spacing, origin[3];
  
  inData->GetWholeExtent(extent);
  inData->GetOrigin(origin);
  spacing = inData->GetSpacing();

  // TranslateExtent the OutputWholeExtent with the input WholeExtent
  for (idx = 0; idx < 3; ++idx)
    {
    // change extent
    extent[2*idx] += this->Translation[idx];
    extent[2*idx+1] += this->Translation[idx];
    // change origin so the data does not shift
    origin[idx] -= (float)(this->Translation[idx]) * spacing[idx];
    }
  
  outData->SetWholeExtent(extent);
  outData->SetOrigin(origin);
}

//----------------------------------------------------------------------------
// This method simply copies by reference the input data to the output.
void vtkImageTranslateExtent::ExecuteData(vtkDataObject *data)
{
  vtkImageData *inData = this->GetInput();
  vtkImageData *outData = (vtkImageData *)(data);
  int extent[6];
  
  // since inData can be larger than update extent.
  inData->GetExtent(extent);
  for (int i = 0; i < 3; ++i)
    {
    extent[i*2] += this->Translation[i];
    extent[i*2+1] += this->Translation[i];
    }
  outData->SetExtent(extent);
  outData->GetPointData()->PassData(inData->GetPointData());
}

//----------------------------------------------------------------------------
void vtkImageTranslateExtent::ComputeInputUpdateExtent(int extent[6], 
                                                       int inExtent[6])
{
  extent[0] = inExtent[0] - this->Translation[0];
  extent[1] = inExtent[1] - this->Translation[0];
  extent[2] = inExtent[2] - this->Translation[1];
  extent[3] = inExtent[3] - this->Translation[1];
  extent[4] = inExtent[4] - this->Translation[2];
  extent[5] = inExtent[5] - this->Translation[2];
}
