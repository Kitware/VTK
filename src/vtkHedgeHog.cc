/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHedgeHog.cc
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
#include "vtkHedgeHog.hh"

void vtkHedgeHog::Execute()
{
  vtkDataSet *input=this->Input;
  int numPts;
  vtkFloatPoints *newPts;
  vtkPointData *pd;
  vtkVectors *inVectors;
  int i, ptId, pts[2];
  vtkCellArray *newLines;
  float *x, *v;
  float newX[3];
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  
  //
  // Initialize
  //

  numPts = input->GetNumberOfPoints();
  pd = input->GetPointData();
  inVectors = pd->GetVectors();
  if ( !inVectors || numPts < 1 )
    {
    vtkErrorMacro(<<"No input data");
    return;
    }
  outputPD->CopyAllocate(pd, 2*numPts);

  newPts = new vtkFloatPoints(2*numPts);
  newLines = new vtkCellArray;
  newLines->Allocate(newLines->EstimateSize(numPts,2));
//
// Loop over all points, creating oriented line
//
  for (ptId=0; ptId < numPts; ptId++)
    {
    x = input->GetPoint(ptId);
    v = inVectors->GetVector(ptId);
    for (i=0; i<3; i++)
      {
      newX[i] = x[i] + this->ScaleFactor * v[i];
      }

    pts[0] = ptId;
    pts[1] = ptId + numPts;;

    newPts->SetPoint(pts[0], x);
    newPts->SetPoint(pts[1], newX);

    newLines->InsertNextCell(2,pts);

    outputPD->CopyData(pd,ptId,pts[0]);
    outputPD->CopyData(pd,ptId,pts[1]);
    }
//
// Update ourselves and release memory
//
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetLines(newLines);
  newLines->Delete();
}

void vtkHedgeHog::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyFilter::PrintSelf(os,indent);

  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
}
