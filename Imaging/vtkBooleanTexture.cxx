/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBooleanTexture.cxx
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
#include "vtkBooleanTexture.h"
#include "vtkUnsignedCharArray.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkBooleanTexture, "1.32");
vtkStandardNewMacro(vtkBooleanTexture);

vtkBooleanTexture::vtkBooleanTexture()
{
  this->Thickness = 0;

  this->XSize = this->YSize = 12;

  this->InIn[0] = this->InIn[1] = 255;
  this->InOut[0] = this->InOut[1] = 255;
  this->OutIn[0] = this->OutIn[1] = 255;
  this->OutOut[0] = this->OutOut[1] = 255;
  this->OnOn[0] = this->OnOn[1] = 255;
  this->OnIn[0] = this->OnIn[1] = 255;
  this->OnOut[0] = this->OnOut[1] = 255;
  this->InOn[0] = this->InOn[1] = 255;
  this->OutOn[0] = this->OutOn[1] = 255;
}

//----------------------------------------------------------------------------
void vtkBooleanTexture::ExecuteInformation()
{
  vtkImageData *output = this->GetOutput();
  
  output->SetWholeExtent(0,this->XSize -1, 0, this->YSize - 1, 0,0);
  output->SetScalarType(VTK_UNSIGNED_CHAR);
  output->SetNumberOfScalarComponents(2);
}

void vtkBooleanTexture::ExecuteData(vtkDataObject *outp)
{
  int i, j;
  int midILower, midJLower, midIUpper, midJUpper;
  vtkImageData *output = this->AllocateOutputData(outp);
  vtkUnsignedCharArray *newScalars = 
    vtkUnsignedCharArray::SafeDownCast(output->GetPointData()->GetScalars());
  
  if (!newScalars || this->XSize*this->YSize < 1 )
    {
    vtkErrorMacro(<<"Bad texture (xsize,ysize) specification!");
    return;
    }

//
// Compute size of various regions
//
  midILower = (int) ((float)(this->XSize - 1) / 2.0 - this->Thickness / 2.0);
  midJLower = (int) ((float)(this->YSize - 1) / 2.0 - this->Thickness / 2.0);
  midIUpper = (int) ((float)(this->XSize - 1) / 2.0 + this->Thickness / 2.0);
  midJUpper = (int) ((float)(this->YSize - 1) / 2.0 + this->Thickness / 2.0);
//
// Create texture map
//
  int count = 0;
  for (j = 0; j < this->YSize; j++) 
    {
    for (i = 0; i < this->XSize; i++) 
      {
      if (i < midILower && j < midJLower) 
        {
        newScalars->SetValue(count,this->InIn[0]);
        count++;
        newScalars->SetValue(count,this->InIn[1]);
        }
      else if (i > midIUpper && j < midJLower) 
        {
        newScalars->SetValue(count,this->OutIn[0]);
        count++;
        newScalars->SetValue(count,this->OutIn[1]);
        }
      else if (i < midILower && j > midJUpper)
        {
        newScalars->SetValue(count,this->InOut[0]);
        count++;
        newScalars->SetValue(count,this->InOut[1]);
        }
      else if (i > midIUpper && j > midJUpper)
        {
        newScalars->SetValue(count,this->OutOut[0]);
        count++;
        newScalars->SetValue(count,this->OutOut[1]);
        }
      else if ((i >= midILower && i <= midIUpper) && (j >= midJLower && j <= midJUpper))
        {
        newScalars->SetValue(count,this->OnOn[0]);
        count++;
        newScalars->SetValue(count,this->OnOn[1]);
        }
      else if ((i >= midILower && i <= midIUpper) && j < midJLower)
        {
        newScalars->SetValue(count,this->OnIn[0]);
        count++;
        newScalars->SetValue(count,this->OnIn[1]);
        }
      else if ((i >= midILower && i <= midIUpper) && j > midJUpper)
        {
        newScalars->SetValue(count,this->OnOut[0]);
        count++;
        newScalars->SetValue(count,this->OnOut[1]);
        }
      else if (i < midILower && (j >= midJLower && j <= midJUpper))
        {
        newScalars->SetValue(count,this->InOn[0]);
        count++;
        newScalars->SetValue(count,this->InOn[1]);
        }
      else if (i > midIUpper && (j >= midJLower && j <= midJUpper))
        {
        newScalars->SetValue(count,this->OutOn[0]);
        count++;
        newScalars->SetValue(count,this->OutOn[1]);
        }
      count++;
      }
    }

}

void vtkBooleanTexture::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "X Size: " << this->XSize << "\n";
  os << indent << "Y Size: " << this->YSize << "\n";

  os << indent << "Thickness: " << this->Thickness << "\n";
  os << indent << "In/In: (" << this->InIn[0] << "," << this->InIn[1] << ")\n";
  os << indent << "In/Out: (" << this->InOut[0] << "," << this->InOut[1] << ")\n";
  os << indent << "Out/In: (" << this->OutIn[0] << "," << this->OutIn[1] << ")\n";
  os << indent << "Out/Out: (" << this->OutOut[0] << "," << this->OutOut[1] << ")\n";
  os << indent << "On/On: (" << this->OnOn[0] << "," << this->OnOn[1] << ")\n";
  os << indent << "On/In: (" << this->OnIn[0] << "," << this->OnIn[1] << ")\n";
  os << indent << "On/Out: (" << this->OnOut[0] << "," << this->OnOut[1] << ")\n";
  os << indent << "In/On: (" << this->InOn[0] << "," << this->InOn[1] << ")\n";
  os << indent << "Out/On: (" << this->OutOn[0] << "," << this->OutOn[1] << ")\n";
}

