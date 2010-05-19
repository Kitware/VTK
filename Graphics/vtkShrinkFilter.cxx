/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShrinkFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkShrinkFilter.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkShrinkFilter);

//----------------------------------------------------------------------------
vtkShrinkFilter::vtkShrinkFilter()
{
  this->ShrinkFactor = 0.5;
  this->GetInformation()->Set(vtkAlgorithm::PRESERVES_RANGES(), 1);
  this->GetInformation()->Set(vtkAlgorithm::PRESERVES_BOUNDS(), 1);
}

//----------------------------------------------------------------------------
vtkShrinkFilter::~vtkShrinkFilter()
{
}

//----------------------------------------------------------------------------
void vtkShrinkFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Shrink Factor: " << this->ShrinkFactor << "\n";
}

//----------------------------------------------------------------------------
int vtkShrinkFilter::FillInputPortInformation(int, vtkInformation* info)
{
  // This filter uses the vtkDataSet cell traversal methods so it
  // suppors any data set type as input.
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkShrinkFilter::RequestData(vtkInformation*,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector)
{
  // Get input and output data.
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0]);
  vtkUnstructuredGrid* output = vtkUnstructuredGrid::GetData(outputVector);

  // We are now executing this filter.
  vtkDebugMacro("Shrinking cells");

  // Skip execution if there is no input geometry.
  vtkIdType numCells = input->GetNumberOfCells();
  vtkIdType numPts = input->GetNumberOfPoints();
  if(numCells < 1 || numPts < 1)
    {
    vtkDebugMacro("No data to shrink!");
    return 1;
    }

  // Allocate working space for new and old cell point lists.
  vtkSmartPointer<vtkIdList> ptIds = vtkSmartPointer<vtkIdList>::New();
  vtkSmartPointer<vtkIdList> newPtIds = vtkSmartPointer<vtkIdList>::New();
  ptIds->Allocate(VTK_CELL_SIZE);
  newPtIds->Allocate(VTK_CELL_SIZE);

  // Allocate approximately the space needed for the output cells.
  output->Allocate(numCells);

  // Allocate space for a new set of points.
  vtkSmartPointer<vtkPoints> newPts = vtkSmartPointer<vtkPoints>::New();
  newPts->Allocate(numPts*8, numPts);

  // Allocate space for data associated with the new set of points.
  vtkPointData* inPD = input->GetPointData();
  vtkPointData* outPD = output->GetPointData();
  outPD->CopyAllocate(inPD, numPts*8, numPts);

  // Support progress and abort.
  vtkIdType tenth = (numCells >= 10? numCells/10 : 1);
  double numCellsInv = 1.0/numCells;
  int abort = 0;
  
  // Point Id map.
  vtkIdType* pointMap = new vtkIdType[input->GetNumberOfPoints()];

  // Traverse all cells, obtaining node coordinates.  Compute "center"
  // of cell, then create new vertices shrunk towards center.
  for(vtkIdType cellId = 0; cellId < numCells && !abort; ++cellId)
    {
    // Get the list of points for this cell.
    input->GetCellPoints(cellId, ptIds);
    vtkIdType numIds = ptIds->GetNumberOfIds();
    
    // Periodically update progress and check for an abort request.
    if(cellId % tenth == 0)
      {
      this->UpdateProgress((cellId+1)*numCellsInv);
      abort = this->GetAbortExecute();
      }

    // Compute the center of mass of the cell points.
    double center[3] = {0,0,0};
    for(vtkIdType i=0; i < numIds; ++i)
      {
      double p[3];
      input->GetPoint(ptIds->GetId(i), p);
      for(int j=0; j < 3; ++j)
        {
        center[j] += p[j];
        }
      }
    for(int j=0; j < 3; ++j)
      {
      center[j] /= numIds;
      }

    // Create new points for this cell.
    newPtIds->Reset();
    for(vtkIdType i=0; i < numIds; ++i)
      {
      // Get the old point location.
      double p[3];
      input->GetPoint(ptIds->GetId(i), p);

      // Compute the new point location.
      double newPt[3];
      for(int j=0; j < 3; ++j)
        {
        newPt[j] = center[j] + this->ShrinkFactor*(p[j] - center[j]);
        }

      // Create the new point for this cell.
      vtkIdType newId = newPts->InsertNextPoint(newPt);

      // Copy point data from the old point.
      vtkIdType oldId = ptIds->GetId(i);
      outPD->CopyData(inPD, oldId, newId);

      pointMap[oldId] = newId;
      }

    // special handling for polyhedron cells
    if (vtkUnstructuredGrid::SafeDownCast(input) &&
        input->GetCellType(cellId) == VTK_POLYHEDRON)
      {
      vtkUnstructuredGrid::SafeDownCast(input)->GetFaceStream(cellId, newPtIds);
      vtkUnstructuredGrid::ConvertFaceStreamPointIds(newPtIds, pointMap);
      }
    else
      {
      for(vtkIdType i=0; i < numIds; ++i)
        {
        newPtIds->InsertId(i, pointMap[ptIds->GetId(i)]);
        }
      }
    
    // Store the new cell in the output.
    output->InsertNextCell(input->GetCellType(cellId), newPtIds);
    }

  // Store the new set of points in the output.
  output->SetPoints(newPts);

  // Just pass cell data through because we still have the same number
  // and type of cells.
  output->GetCellData()->PassData(input->GetCellData());

  // Avoid keeping extra memory around.
  output->Squeeze();

  delete [] pointMap;
  
  return 1;
}
