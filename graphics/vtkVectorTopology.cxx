/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectorTopology.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include <math.h>
#include "vtkVectorTopology.h"

// Description:
// Construct object with distance 0.1.
vtkVectorTopology::vtkVectorTopology()
{
  this->Distance = 0.1;
}

void vtkVectorTopology::Execute()
{
  int cellId, i, j, ptId, npts;
  int negative[3], positive[3], subId=0;
  float x[3], pcoords[3], *v;
  vtkCell *cell;
  vtkVectors *inVectors;
  vtkFloatPoints *newPts;
  vtkCellArray *newVerts;
  vtkDataSet *input=this->Input;
  vtkPointData *pd=input->GetPointData();
  vtkPolyData *output=(vtkPolyData *)this->Output;
  vtkPointData *outputPD=output->GetPointData();
  float *weights=new float[input->GetMaxCellSize()];
//
// Initialize self; check input; create output objects
//
  vtkDebugMacro(<< "Executing vector topology...");

  // make sure we have vector data
  if ( ! (inVectors = input->GetPointData()->GetVectors()) )
    {
    vtkErrorMacro(<<"No vector data, can't create topology markers...");
    return;
    }

  newPts = vtkFloatPoints::New();
  newPts->Allocate(100);
  newVerts = vtkCellArray::New();
  newVerts->Allocate(newVerts->EstimateSize(1,100));
  outputPD->CopyAllocate(pd);
//
// Find cells whose vector components all pass through zero
//
  pcoords[0] = pcoords[1] = pcoords[2] = 0.5;
  newVerts->InsertNextCell(100); //temporary count
  for (cellId=0; cellId<Input->GetNumberOfCells(); cellId++)
    {
    cell = Input->GetCell(cellId);
    npts = cell->GetNumberOfPoints();
    for (i=0; i<3; i++) negative[i] = positive[i] = 0;
    for (i=0; i < npts; i++)
      {
      ptId = cell->GetPointId(i);
      v = inVectors->GetVector(ptId);
      for (j=0; j<3; j++)
        {
        if ( v[j] < 0.0 ) negative[j] = 1;
        else if ( v[j] >= 0.0 ) positive[j] = 1;
        }
      }
    if ( negative[0] && positive[0] && negative[1] && positive[1] &&
    negative[2] && positive[2] )
      { // place point at center of cell
      cell->EvaluateLocation(subId, pcoords, x, weights);
      ptId = newPts->InsertNextPoint(x);
      newVerts->InsertCellPoint(ptId);
      }
    }
  newVerts->UpdateCellCount(newPts->GetNumberOfPoints());
 
  vtkDebugMacro(<< "Created " << newPts->GetNumberOfPoints() << "points");
  delete [] weights;
//
// Update ourselves
//
  output->SetPoints(newPts);
  output->SetVerts(newVerts);
  output->Squeeze();
}

void vtkVectorTopology::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Distance: " << this->Distance << "\n";
}


