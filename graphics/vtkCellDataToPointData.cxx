/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellDataToPointData.cxx
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
#include "vtkCellDataToPointData.h"

// Description:
// Instantiate object so that cell data is not passed to output.
vtkCellDataToPointData::vtkCellDataToPointData()
{
  this->PassCellData = 0;
}

#define VTK_MAX_CELLS_PER_POINT 4096

void vtkCellDataToPointData::Execute()
{
  int cellId, ptId;
  int numCells, numPts;
  vtkDataSet *input=(vtkDataSet *)this->Input;
  vtkDataSet *output=(vtkDataSet *)this->Output;
  vtkCellData *inPD=input->GetCellData();
  vtkPointData *outPD=output->GetPointData();
  int maxCellSize=input->GetMaxCellSize();
  vtkIdList cellIds(VTK_MAX_CELLS_PER_POINT);
  float weight, *weights=new float[VTK_MAX_CELLS_PER_POINT];

  vtkDebugMacro(<<"Mapping cell data to point data");

  if ( (numPts=input->GetNumberOfPoints()) < 1 )
    {
    vtkErrorMacro(<<"No input point data!");
    return;
    }
  
  // notice that inPD and outPD are vtkCellData and vtkPointData; respectively.
  // It's weird, but it works.
  outPD->CopyAllocate(inPD,numPts);

  for (ptId=0; ptId < numPts; ptId++)
    {
    input->GetPointCells(ptId, cellIds);
    numCells = cellIds.GetNumberOfIds();
    if ( numCells > 0 )
      {
      weight = 1.0 / numCells;
      for (cellId=0; cellId < numCells; cellId++) weights[cellId] = weight;
      outPD->InterpolatePoint(inPD, ptId, &cellIds, weights);
      }
    else
      {
      outPD->NullPoint(ptId);
      }
    }

  if ( this->PassCellData )
    {
    output->GetCellData()->PassData(input->GetCellData());
    }
  
  delete [] weights;
}

void vtkCellDataToPointData::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Pass Cell Data: " << (this->PassCellData ? "On\n" : "Off\n");
}
