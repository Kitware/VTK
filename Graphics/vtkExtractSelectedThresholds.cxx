/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectedThresholds.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractSelectedThresholds.h"

#include "vtkDataSet.h"
#include "vtkThreshold.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkUnstructuredGrid.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"

vtkCxxRevisionMacro(vtkExtractSelectedThresholds, "1.3");
vtkStandardNewMacro(vtkExtractSelectedThresholds);

//----------------------------------------------------------------------------
vtkExtractSelectedThresholds::vtkExtractSelectedThresholds()
{
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
vtkExtractSelectedThresholds::~vtkExtractSelectedThresholds()
{
}

//----------------------------------------------------------------------------
int vtkExtractSelectedThresholds::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *selInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the selection, input and ouptut
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkSelection *sel = vtkSelection::SafeDownCast(
    selInfo->Get(vtkDataObject::DATA_OBJECT()));
  if ( ! sel )
    {
    vtkErrorMacro(<<"No selection specified");
    return 1;
    }

  if (!sel->GetProperties()->Has(vtkSelection::CONTENT_TYPE()) ||
      sel->GetProperties()->Get(vtkSelection::CONTENT_TYPE()) != vtkSelection::THRESHOLDS)
    {
    return 1;
    }
  vtkDebugMacro(<< "Extracting from dataset");

  //find the values to threshold within
  vtkDoubleArray *lims = vtkDoubleArray::SafeDownCast(sel->GetSelectionList());
  if (lims == NULL)
    {
    vtkErrorMacro(<<"No values to threshold with");
    return 1;
    }

  //find out what array we are suppose to threshold in
  vtkDataArray *inScalars = NULL;
  if (
    sel->GetProperties()->Has(vtkSelection::FIELD_TYPE()) &&
    (sel->GetProperties()->Get(vtkSelection::FIELD_TYPE()) == 
     vtkSelection::POINT)     
    )
    {
    if (sel->GetProperties()->Has(vtkSelection::NAME()))
      {
      inScalars = input->GetPointData()->GetArray(
        sel->GetProperties()->Get(vtkSelection::NAME())
        );      
      }
    else
      {
      inScalars = input->GetPointData()->GetScalars();
      }
    }
  else
    {
    if (sel->GetProperties()->Has(vtkSelection::NAME()))
      {
      inScalars = input->GetCellData()->GetArray(
        sel->GetProperties()->Get(vtkSelection::NAME())
        );      
      }
    else
      {
      inScalars = input->GetCellData()->GetScalars();
      }
    }
  if (inScalars == NULL)
    {
    vtkErrorMacro("Could not figure out what array to thresholds in.");
    return 1;
    }
  

  vtkIdType cellId, newCellId;
  vtkIdList *cellPts, *pointMap;
  vtkIdList *newCellPts;
  vtkCell *cell;
  vtkPoints *newPoints;
  int i, ptId, newId, numPts;
  int numCellPts;
  double x[3];
  vtkPointData *pd=input->GetPointData(), *outPD=output->GetPointData();
  vtkCellData *cd=input->GetCellData(), *outCD=output->GetCellData();
  int keepCell, usePointScalars;

  vtkIdTypeArray *originalCellIds = vtkIdTypeArray::New();
  outCD->AddArray(originalCellIds);
  originalCellIds->SetName("vtkOriginalCellIds");
  originalCellIds->SetNumberOfComponents(1);

  outPD->CopyGlobalIdsOn();
  outPD->CopyAllocate(pd);
  outCD->CopyGlobalIdsOn();
  outCD->CopyAllocate(cd);

  numPts = input->GetNumberOfPoints();
  output->Allocate(input->GetNumberOfCells());
  newPoints = vtkPoints::New();
  newPoints->Allocate(numPts);

  pointMap = vtkIdList::New(); //maps old point ids into new
  pointMap->SetNumberOfIds(numPts);
  for (i=0; i < numPts; i++)
    {
    pointMap->SetId(i,-1);
    }

  newCellPts = vtkIdList::New();     

  // are we using pointScalars?
  usePointScalars = (inScalars->GetNumberOfTuples() == numPts);
  
  // Check that the scalars of each cell satisfy the threshold criterion
  for (cellId=0; cellId < input->GetNumberOfCells(); cellId++)
    {
    cell = input->GetCell(cellId);
    cellPts = cell->GetPointIds();
    numCellPts = cell->GetNumberOfPoints();
    
    if ( usePointScalars )
      {
      keepCell = 0;
      for ( i=0; (!keepCell) && (i < numCellPts); i++)
        {
        ptId = cellPts->GetId(i);
        keepCell = this->EvaluateValue( inScalars, ptId, lims );
        }
      }
    else //use cell scalars
      {
      keepCell = this->EvaluateValue( inScalars, cellId, lims );
      }
    
    if (  numCellPts > 0 && keepCell )
      {
      // satisfied thresholding (also non-empty cell, i.e. not VTK_EMPTY_CELL)
      originalCellIds->InsertNextValue(cellId);

      for (i=0; i < numCellPts; i++)
        {
        ptId = cellPts->GetId(i);
        if ( (newId = pointMap->GetId(ptId)) < 0 )
          {
          input->GetPoint(ptId, x);
          newId = newPoints->InsertNextPoint(x);
          pointMap->SetId(ptId,newId);
          outPD->CopyData(pd,ptId,newId);
          }
        newCellPts->InsertId(i,newId);
        }
      newCellId = output->InsertNextCell(cell->GetCellType(),newCellPts);
      outCD->CopyData(cd,cellId,newCellId);
      newCellPts->Reset();
      } // satisfied thresholding
    } // for all cells
  
  vtkDebugMacro( << "Extracted " << output->GetNumberOfCells() << " number of cells.");

  // now clean up / update ourselves
  pointMap->Delete();
  newCellPts->Delete();
  originalCellIds->Delete();
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
void vtkExtractSelectedThresholds::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

}

//----------------------------------------------------------------------------
int vtkExtractSelectedThresholds::FillInputPortInformation(
  int port, vtkInformation* info)
{
  if (port==0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    }
  else
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");    
    }
  return 1;
}
  
//----------------------------------------------------------------------------
int vtkExtractSelectedThresholds::EvaluateValue(
  vtkDataArray *scalars, vtkIdType id, vtkDoubleArray *lims)
{
  int keepCell = 0;
  //check the value in the array against all of the thresholds in lims
  //if it is inside any, return true
  for (int i = 0; i < lims->GetNumberOfTuples(); i+=2)
    {
    double value = scalars->GetComponent(id, 0);
    if (value >= lims->GetValue(i) && value <= lims->GetValue(i+1))
      {
      keepCell = 1;
      }
    }
  return keepCell;
}
