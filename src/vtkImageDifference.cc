/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDifference.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
#include "vtkImageDifference.hh"
#include "vtkPixmap.hh"
#include "stdlib.h"

// Description:
// Construct object to extract all of the input data.
vtkImageDifference::vtkImageDifference()
{
  this->Image = NULL;
}

// simple macro for calculating error
#define vtkImageDifferenceCalcError(c1,c2) \
  r1 = abs(c1[0] - c2[0]); g1 = abs(c1[1] - c2[1]); b1 = abs(c1[2] - c2[2]); \
  if ((r1+g1+b1) < (tr+tg+tb)) { tr = r1; tg = g1; tb = b1; }

void vtkImageDifference::Execute()
{
  vtkStructuredPoints *input = (vtkStructuredPoints *)this->Input;
  vtkPointData *pd1 = input->GetPointData();
  vtkPointData *pd2 = this->Image->GetPointData();
  vtkStructuredPoints *output=(vtkStructuredPoints *)this->Output;
  vtkPointData *outPD=output->GetPointData();
  int *dims1, *dims2, sliceSize;
  int row, col, idx;
  vtkColorScalars *s1, *s2;
  unsigned char *color1, color2[4], outColor[4];
  int tr, tg, tb, r1, g1, b1;
  int threshold = 51;
  vtkPixmap *outScalars = new vtkPixmap;
  
  vtkDebugMacro(<< "Comparing Images");

  dims1 = input->GetDimensions();
  dims2 = this->Image->GetDimensions();
  if ((dims1[0] != dims2[0]) || 
      (dims1[1] != dims2[1]) || 
      (dims1[2] != dims2[2]))
    {
    vtkWarningMacro(<< "Images are not the same size");
    this->Error = 1;
    this->ThresholdedError = 1;
    return;
    }

  // make sure the images are of the correct type
  if (strcmp(pd1->GetScalars()->GetScalarType(),"ColorScalar") ||
      strcmp(pd2->GetScalars()->GetScalarType(),"ColorScalar"))
    {
    vtkWarningMacro(<< "Scalars must be of type ColorScalar.");
    return;
    }

  s1 = (vtkColorScalars *)pd1->GetScalars();
  s2 = (vtkColorScalars *)pd2->GetScalars();
  outColor[3] = 255;
  
  //
  // Allocate necessary objects
  //
  output->SetDimensions(dims1);
  sliceSize = dims1[0]*dims1[1];

  this->Error = 0;
  this->ThresholdedError = 0;
  
  for (row = 0; row < dims1[1]; row++)
    {
    idx = row*dims1[0];
    for (col = 0; col < dims1[0]; col++)
      {
      tr = 1000;
      tg = 1000;
      tb = 1000;
      s2->GetColor(idx+col,color2);
      
      /* check the exact match pixel */
      color1 = s1->GetColor(idx+col);
      vtkImageDifferenceCalcError(color1,color2);
	
      /* check the pixel to the left */
      if (col)
	{
	color1 = s1->GetColor(idx + col - 1);
	vtkImageDifferenceCalcError(color1,color2);
	}
	
	/* check the pixel to the right */
      if (col < (dims1[0] -1))
	{
	color1 = s1->GetColor(idx + col + 1);
	vtkImageDifferenceCalcError(color1,color2);
	}
      
      /* check the line above if there is one */
      if (row)
	{
	/* check the exact match pixel */
	color1 = s1->GetColor(idx - dims1[0] + col);
	vtkImageDifferenceCalcError(color1,color2);
	
	/* check the pixel to the left */
	if (col)
	  {
	  color1 = s1->GetColor(idx - dims1[0] + col - 1);
	  vtkImageDifferenceCalcError(color1,color2);
	  }
	  
	/* check the pixel to the right */
	if (col < (dims1[0] -1))
	  {
	  color1 = s1->GetColor(idx - dims1[0] + col + 1);
	  vtkImageDifferenceCalcError(color1,color2);
	  }
	}
      
      /* check the line below if there is one */
      if (row < (dims1[1] - 1))
	{
	/* check the exact match pixel */
	color1 = s1->GetColor(idx + dims1[0] + col);
	vtkImageDifferenceCalcError(color1,color2);
	
	/* check the pixel to the left */
	if (col)
	  {
	  color1 = s1->GetColor(idx + dims1[0] + col - 1);
	  vtkImageDifferenceCalcError(color1,color2);
	  }
	
	/* check the pixel to the right */
	if (col < (dims1[0] -1))
	  {
	  color1 = s1->GetColor(idx + dims1[0] + col + 1);
	  vtkImageDifferenceCalcError(color1,color2);
	  }
	}
      
      this->Error = this->Error + (tr + tg + tb)/(3.0*255);
      tr -= threshold;
      if (tr < 0) tr = 0;
      tg -= threshold;
      if (tg < 0) tg = 0;
      tb -= threshold;
      if (tb < 0) tb = 0;
      outColor[0] = tr;
      outColor[1] = tg;
      outColor[2] = tb;
      this->ThresholdedError = 
	this->ThresholdedError + (tr + tg + tb)/(3.0*255.0);
      outScalars->InsertNextColor(outColor);
      }
    }

  this->Error = this->Error/sliceSize;
  this->ThresholdedError = this->ThresholdedError/sliceSize;

  outPD->SetScalars(outScalars);
  outScalars->Delete();
}


void vtkImageDifference::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsFilter::PrintSelf(os,indent);
  
  os << indent << "Error: " << this->Error << "\n";
  os << indent << "ThresholdedError: " << this->ThresholdedError << "\n";
}


// Description:
// Override update method because execution can branch two ways (Input 
// and Image)
void vtkImageDifference::Update()
{
  // make sure input is available
  if ( this->Input == NULL || this->Image == NULL)
    {
    vtkErrorMacro(<< "No input...can't execute!");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  this->Input->Update();
  this->Image->Update();
  this->Updating = 0;

  if (this->Input->GetMTime() > this->ExecuteTime || 
      this->Image->GetMTime() > this->ExecuteTime || 
      this->GetMTime() > this->ExecuteTime || this->GetDataReleased() )
    {
    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    this->Output->Initialize(); //clear output
    this->Execute();
    this->ExecuteTime.Modified();
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }

  if ( this->Input->ShouldIReleaseData() ) this->Input->ReleaseData();
  if ( this->Image->ShouldIReleaseData() ) this->Image->ReleaseData();
}
