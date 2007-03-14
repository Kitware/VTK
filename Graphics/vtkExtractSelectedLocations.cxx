/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectedLocations.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractSelectedLocations.h"

#include "vtkDataSet.h"
#include "vtkPoints.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkCell.h"

vtkCxxRevisionMacro(vtkExtractSelectedLocations, "1.1");
vtkStandardNewMacro(vtkExtractSelectedLocations);

//----------------------------------------------------------------------------
vtkExtractSelectedLocations::vtkExtractSelectedLocations()
{
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
vtkExtractSelectedLocations::~vtkExtractSelectedLocations()
{
}

//----------------------------------------------------------------------------
int vtkExtractSelectedLocations::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *selInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
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
      sel->GetProperties()->Get(vtkSelection::CONTENT_TYPE()) != vtkSelection::LOCATIONS)
    {
    return 1;
    }

  vtkDebugMacro(<< "Extracting from dataset");

  //get a hold of input data structures and allocate output data structures
  vtkDoubleArray *selLocations = 
    vtkDoubleArray::SafeDownCast(sel->GetSelectionList());
  vtkIdType numSLocations = selLocations->GetNumberOfTuples();

  vtkIdType numILocations = input->GetNumberOfPoints();
  vtkPointData *inPD = input->GetPointData();
  vtkIdType numICells = input->GetNumberOfCells();
  vtkCellData *inCD = input->GetCellData();

  vtkPoints *newPts = vtkPoints::New();
  newPts->Allocate(numSLocations*4);
  output->Allocate(numSLocations);
  vtkPointData *outputPD = output->GetPointData();
  outputPD->CopyAllocate(inPD);
  vtkCellData *outputCD = output->GetCellData();
  outputCD->CopyAllocate(inCD);

  vtkIdTypeArray *originalCellIds = vtkIdTypeArray::New();
  outputCD->AddArray(originalCellIds);
  originalCellIds->SetName("vtkOriginalCellIds");
  originalCellIds->SetNumberOfComponents(1);

  //find the cells in the input that contain the locations in the selection
  //create a map to convert old to new point ids
  vtkIdType *pointMap = new vtkIdType[numILocations]; 
  vtkIdType ptId, newPointId;
  for (ptId=0; ptId < numILocations; ptId++)
    {
    pointMap[ptId] = -1;
    }

  //information about of the current cell
  vtkCell* cell;
  vtkIdType numCellPts;
  vtkIdList *cellPtIds;
  vtkIdList *newCellPtIds = vtkIdList::New();
  newCellPtIds->Allocate(VTK_CELL_SIZE);
  double x[3];

  //parameters for the is pt inside cell method
  int inside;
  double closestPoint[3];
  int subId;
  double pcoords[3];
  double dist2;
  double* weights = new double[input->GetMaxCellSize()];

  for (vtkIdType c = 0; c < numICells; c++)
    {
    cell = input->GetCell(c);
    int cellD = cell->GetCellDimension();

    for (vtkIdType p = 0; p < numSLocations; p++)
      {    
      double *point = selLocations->GetTuple(p);

      inside = cell->EvaluatePosition(
        point,
        closestPoint, subId, pcoords, dist2, weights);
      
      if (inside == 1 && dist2==0.0) //have to check dist for 2D cells
        {
        //copy over the locations that make up the cell
        cellPtIds = cell->GetPointIds();
        numCellPts = cell->GetNumberOfPoints();    
        newCellPtIds->Reset();
        //intersects, put all of the locations inside
        for (vtkIdType i=0; i < numCellPts; i++)
          {
          ptId = cellPtIds->GetId(i);
          newPointId = pointMap[ptId];
          if (newPointId < 0)
            {             
            input->GetPoint(ptId, x);      
            newPointId = newPts->InsertNextPoint(x);
            outputPD->CopyData(inPD,ptId,newPointId);
            pointMap[ptId] = newPointId;
            }
          newCellPtIds->InsertId(i,newPointId);
          }
        vtkIdType newCellId = 
          output->InsertNextCell(cell->GetCellType(),newCellPtIds);
        outputCD->CopyData(inCD,c,newCellId);
        originalCellIds->InsertNextValue(c);
        break;
        }
      }
    }

  output->SetPoints(newPts);
  output->Squeeze();

  delete[] pointMap;
  newPts->Delete();  
  newCellPtIds->Delete();
  delete[] weights;  
  originalCellIds->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkExtractSelectedLocations::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
int vtkExtractSelectedLocations::FillInputPortInformation(
  int port, vtkInformation* info)
{
  if (port==0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");    
    }
  else
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    }
  return 1;
}
