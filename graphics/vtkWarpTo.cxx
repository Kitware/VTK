/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpTo.cxx
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
#include "vtkWarpTo.h"
#include "vtkMath.h"

void vtkWarpTo::Execute()
{
  vtkPoints *inPts;
  vtkFloatPoints *newPts;
  vtkPointData *pd;
  int i, ptId;
  float *x, newX[3];
  vtkPointSet *input=(vtkPointSet *)this->Input;
  vtkPointSet *output=(vtkPointSet *)this->Output;
  float mag;
  float minMag = 0;
  
  vtkDebugMacro(<<"Warping data to a point");

  inPts = input->GetPoints();
  pd = input->GetPointData();

  if (!inPts )
    {
    vtkErrorMacro(<<"No input data");
    return;
    }

  newPts = new vtkFloatPoints(inPts->GetNumberOfPoints());

  if (this->Absolute)
    {
    minMag = 1.0e10;
    for (ptId=0; ptId < inPts->GetNumberOfPoints(); ptId++)
      {
      x = inPts->GetPoint(ptId);
      mag = sqrt(vtkMath::Distance2BetweenPoints(this->Position,x));
      if (mag < minMag) minMag = mag;
      }
    }
  
  //
  // Loop over all points, adjusting locations
  //
  for (ptId=0; ptId < inPts->GetNumberOfPoints(); ptId++)
    {
    x = inPts->GetPoint(ptId);
    if (this->Absolute)
      {
      mag = sqrt(vtkMath::Distance2BetweenPoints(this->Position,x));
      for (i=0; i<3; i++)
	{
	newX[i] = this->ScaleFactor*
	  (this->Position[i] + minMag*(x[i] - this->Position[i])/mag) + 
	  (1.0 - this->ScaleFactor)*x[i];
	}
      }
    else
      {
      for (i=0; i<3; i++)
	{
	newX[i] = (1.0 - this->ScaleFactor)*x[i] + 
	  this->ScaleFactor*this->Position[i];
	}
      }
    newPts->SetPoint(ptId, newX);
    }
  //
  // Update ourselves and release memory
  //
  output->GetPointData()->CopyNormalsOff(); // distorted geometry
  output->GetPointData()->PassData(input->GetPointData());

  output->SetPoints(newPts);
  newPts->Delete();
}

void vtkWarpTo::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPointSetToPointSetFilter::PrintSelf(os,indent);
  
  os << indent << "Position: (" << this->Position[0] << ", " 
    << this->Position[1] << ", " << this->Position[2] << ")\n";
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
}
