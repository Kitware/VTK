/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBooleanTexture.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkBooleanTexture.hh"
#include "vtkAGraymap.hh"

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

void vtkBooleanTexture::Execute()
{
  int numPts, i, j;
  vtkAGraymap *newScalars;
  int midILower, midJLower, midIUpper, midJUpper;
  vtkStructuredPoints *output = this->GetOutput();
  
  if ( (numPts = this->XSize * this->YSize) < 1 )
    {
    vtkErrorMacro(<<"Bad texture (xsize,ysize) specification!");
    return;
    }

  output->SetDimensions(this->XSize,this->YSize,1);
  newScalars = new vtkAGraymap(numPts);
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
  for (j = 0; j < this->YSize; j++) 
    {
    for (i = 0; i < this->XSize; i++) 
      {
      if (i < midILower && j < midJLower) 
        newScalars->InsertNextAGrayValue(this->InIn);
      else if (i > midIUpper && j < midJLower) 
        newScalars->InsertNextAGrayValue(this->OutIn);
      else if (i < midILower && j > midJUpper) 
        newScalars->InsertNextAGrayValue(this->InOut);
      else if (i > midIUpper && j > midJUpper) 
        newScalars->InsertNextAGrayValue(this->OutOut);
      else if ((i >= midILower && i <= midIUpper) && (j >= midJLower && j <= midJUpper)) 
        newScalars->InsertNextAGrayValue(this->OnOn);
      else if ((i >= midILower && i <= midIUpper) && j < midJLower) 
        newScalars->InsertNextAGrayValue(this->OnIn);
      else if ((i >= midILower && i <= midIUpper) && j > midJUpper) 
        newScalars->InsertNextAGrayValue(this->OnOut);
      else if (i < midILower && (j >= midJLower && j <= midJUpper)) 
        newScalars->InsertNextAGrayValue(this->InOn);
      else if (i > midIUpper && (j >= midJLower && j <= midJUpper)) 
        newScalars->InsertNextAGrayValue(this->OutOn);
      }
    }

//
// Update ourselves
//
  output->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();
}

void vtkBooleanTexture::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsSource::PrintSelf(os,indent);

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

