/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointDataToCellData.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkPointDataToCellData.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkPointDataToCellData* vtkPointDataToCellData::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPointDataToCellData");
  if(ret)
    {
    return (vtkPointDataToCellData*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPointDataToCellData;
}




// Instantiate object so that point data is not passed to output.
vtkPointDataToCellData::vtkPointDataToCellData()
{
  this->PassPointData = 0;
}

void vtkPointDataToCellData::Execute()
{
  int cellId, ptId;
  int numCells, numPts;
  vtkDataSet *input= this->GetInput();
  vtkDataSet *output= this->GetOutput();
  vtkPointData *inPD=input->GetPointData();
  vtkCellData *outPD=output->GetCellData();
  int maxCellSize=input->GetMaxCellSize();
  vtkIdList *cellPts;
  float weight, *weights=new float[maxCellSize];

  vtkDebugMacro(<<"Mapping point data to cell data");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  if ( (numCells=input->GetNumberOfCells()) < 1 )
    {
    vtkErrorMacro(<<"No input cells!");
    return;
    }
  
  cellPts = vtkIdList::New();
  cellPts->Allocate(maxCellSize);

  // notice that inPD and outPD are vtkPointData and vtkCellData; respectively.
  // It's weird, but it works.
  outPD->CopyAllocate(inPD,numCells);

  for (cellId=0; cellId < numCells; cellId++)
    {
    input->GetCellPoints(cellId, cellPts);
    numPts = cellPts->GetNumberOfIds();
    if ( numPts > 0 )
      {
      weight = 1.0 / numPts;
      for (ptId=0; ptId < numPts; ptId++)
	{
	weights[ptId] = weight;
	}
      outPD->InterpolatePoint(inPD, cellId, cellPts, weights);
      }
    }

  // Pass through any cell data that's in the input 
  // and not defined in the output.
  output->GetCellData()->PassNoReplaceData(input->GetCellData());
  
  if ( this->PassPointData )
    {
    output->GetPointData()->PassData(input->GetPointData());
    }
  
  cellPts->Delete();
  delete [] weights;
}

void vtkPointDataToCellData::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Pass Point Data: " << (this->PassPointData ? "On\n" : "Off\n");
}
