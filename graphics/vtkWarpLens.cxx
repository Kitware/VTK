/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpLens.cxx
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
#include "vtkWarpLens.h"
#include "vtkMath.h"

vtkWarpLens::vtkWarpLens()
{
  this->Kappa = -1.0e-6;
  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;
}

void vtkWarpLens::Execute()
{
  vtkPoints *inPts;
  vtkFloatPoints *newPts;
  vtkPointData *pd;
  int i, ptId, numPts;
  float *x, newX[3];
  vtkPointSet *input=(vtkPointSet *)this->Input;
  vtkPointSet *output=(vtkPointSet *)this->Output;
  float r;
  float offset;
  
  vtkDebugMacro(<<"Warping data to a point");

  r = this->Center[0]*this->Center[0];
  offset = this->Center[0]*(1.0 + Kappa*r);

  inPts = input->GetPoints();  
  numPts = inPts->GetNumberOfPoints();
  pd = input->GetPointData();

  if (!inPts )
    {
    vtkErrorMacro(<<"No input data");
    return;
    }

  newPts = new vtkFloatPoints(numPts); newPts->SetNumberOfPoints(numPts);

  //
  // Loop over all points, adjusting locations
  //
  for (ptId=0; ptId < numPts; ptId++)
    {
    x = inPts->GetPoint(ptId);
    newX[0] = x[0] - this->Center[0];
    newX[1] = x[1] - this->Center[1];
    r = newX[0]*newX[0] + newX[1]*newX[1];
    newX[0] = offset + newX[0]*(1.0 + Kappa*r);
    newX[1] = offset + newX[1]*(1.0 + Kappa*r);
    newX[2] = x[2];
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

void vtkWarpLens::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPointSetToPointSetFilter::PrintSelf(os,indent);
  
  os << indent << "Center: (" << this->Center[0] << ", " 
    << this->Center[1] << ", " << this->Center[2] << ")\n";
  os << indent << "Kappa: " << this->Kappa << "\n";
}
