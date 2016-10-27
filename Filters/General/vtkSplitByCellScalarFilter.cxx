/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSplitByCellScalarFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSplitByCellScalarFilter.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <map>
#include <sstream>
#include <vector>

vtkStandardNewMacro(vtkSplitByCellScalarFilter);

//----------------------------------------------------------------------------
vtkSplitByCellScalarFilter::vtkSplitByCellScalarFilter()
{
  this->PassAllPoints = true;
  // by default process active cells scalars
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS,
                               vtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
vtkSplitByCellScalarFilter::~vtkSplitByCellScalarFilter()
{
}

//----------------------------------------------------------------------------
int vtkSplitByCellScalarFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the input and output
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0], 0);
  vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outputVector, 0);

  vtkDataArray *inScalars = this->GetInputArrayToProcess(0, inputVector);

  if (!inScalars)
  {
    vtkErrorMacro(<< "No scalar data to process.");
    return 1;
  }

  double range[2];
  inScalars->GetRange(range);

  vtkIdType nbCells = input->GetNumberOfCells();

  // Fetch the existing scalar ids
  std::map<vtkIdType, int> scalarValuesToBlockId;
  int nbBlocks = 0;
  for (vtkIdType i = 0; i < nbCells; i++)
  {
    vtkIdType v = static_cast<vtkIdType>(inScalars->GetTuple1(i));
    if (scalarValuesToBlockId.find(v) == scalarValuesToBlockId.end())
    {
      scalarValuesToBlockId[v] = nbBlocks;
      nbBlocks++;
    }
  }
  if (nbBlocks == 0)
  {
    vtkDebugMacro(<< "No block found.");
    return 1;
  }

  vtkPointData *inPD = input->GetPointData();
  vtkCellData *inCD = input->GetCellData();
  vtkPointSet *inputPS = vtkPointSet::SafeDownCast(input);
  vtkPolyData* inputPD = vtkPolyData::SafeDownCast(input);
  vtkUnstructuredGrid* inputUG = vtkUnstructuredGrid::SafeDownCast(input);

  // Create one UnstructuredGrid block per scalar ids
  std::vector<vtkPointSet*> blocks(nbBlocks);
  std::map<vtkIdType,int >::const_iterator it = scalarValuesToBlockId.begin();
  bool passAllPoints = inputPS && inputPS->GetPoints() && this->PassAllPoints;
  for (int i = 0; i < nbBlocks; i++, ++it)
  {
    vtkSmartPointer<vtkPointSet> ds;
    ds.TakeReference(inputPD ?
      static_cast<vtkPointSet*>(vtkPolyData::New()) :
      static_cast<vtkPointSet*>(vtkUnstructuredGrid::New()));
    if(passAllPoints)
    {
      ds->SetPoints(inputPS->GetPoints());
      ds->GetPointData()->ShallowCopy(inPD);
    }
    else
    {
      vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
      points->SetDataTypeToDouble();
      ds->SetPoints(points);
      ds->GetPointData()->CopyGlobalIdsOn();
      ds->GetPointData()->CopyAllocate(inPD);
    }
    if (inputPD)
    {
      vtkPolyData::SafeDownCast(ds)->Allocate();
    }
    ds->GetCellData()->CopyGlobalIdsOn();
    ds->GetCellData()->CopyAllocate(inCD);
    blocks[i] = ds;
    output->SetBlock(i, ds);
    std::stringstream ss;
    ss << inScalars->GetName() << "_" << it->first;
    output->GetMetaData(i)->Set(vtkCompositeDataSet::NAME(), ss.str().c_str());
  }

  vtkSmartPointer<vtkIdList> newCellPts = vtkSmartPointer<vtkIdList>::New();
  std::vector<std::map<vtkIdType, vtkIdType> > pointMaps(nbBlocks);

  int abortExecute = this->GetAbortExecute();
  vtkIdType progressInterval = nbCells / 100;

  // Check that the scalars of each cell satisfy the threshold criterion
  for (vtkIdType cellId = 0; cellId < nbCells && !abortExecute; cellId++)
  {
    if (cellId % progressInterval == 0)
    {
      this->UpdateProgress(static_cast<double>(cellId) / nbCells);
      abortExecute = this->GetAbortExecute();
    }
    int cellType = input->GetCellType(cellId);
    vtkIdType v = static_cast<vtkIdType>(inScalars->GetTuple1(cellId));
    int cellBlock = scalarValuesToBlockId[v];
    vtkPointSet* outDS = blocks[cellBlock];
    vtkPolyData* outPD = vtkPolyData::SafeDownCast(outDS);
    vtkUnstructuredGrid* outUG = vtkUnstructuredGrid::SafeDownCast(outDS);
    std::map<vtkIdType, vtkIdType>& pointMap = pointMaps[cellBlock];
    vtkCell* cell = input->GetCell(cellId);
    vtkIdList* cellPts = cell->GetPointIds();

    if (!passAllPoints)
    {
      vtkPointData* outPData = outDS->GetPointData();
      vtkPoints* outPoints = outDS->GetPoints();
      vtkIdType numCellPts = cellPts->GetNumberOfIds();
      newCellPts->Reset();
      for (vtkIdType i = 0; i < numCellPts; i++)
      {
        vtkIdType ptId = cellPts->GetId(i);
        std::map<vtkIdType, vtkIdType>::const_iterator ptIt = pointMap.find(ptId);
        vtkIdType newId = -1;
        if (ptIt == pointMap.end())
        {
          double x[3];
          input->GetPoint(ptId, x);
          newId = outPoints->InsertNextPoint(x);
          pointMap[ptId] = newId;
          outPData->CopyData(inPD, ptId, newId);
        }
        else
        {
          newId = ptIt->second;
        }
        newCellPts->InsertId(i, newId);
      }
    }

    vtkIdType newCellId = -1;
    // special handling for polyhedron cells
    if (inputUG && cellType == VTK_POLYHEDRON)
    {
      inputUG->GetFaceStream(cellId, newCellPts.Get());
      if (!passAllPoints)
      {
        // ConvertFaceStreamPointIds using local point map
        vtkIdType* idPtr = newCellPts->GetPointer(0);
        vtkIdType nfaces = *idPtr++;
        for (vtkIdType i = 0; i < nfaces; i++)
        {
          vtkIdType npts = *idPtr++;
          for (vtkIdType j = 0; j < npts; j++)
          {
            *idPtr = pointMap[*idPtr];
            idPtr++;
          }
        }
      }
      newCellId = outUG->InsertNextCell(cellType, newCellPts);
      newCellPts->Reset();
    }
    else
    {
      if (outPD)
      {
        newCellId = outPD->InsertNextCell(cellType, passAllPoints ? cellPts : newCellPts.Get());
      }
      else
      {
        newCellId = outUG->InsertNextCell(cellType, passAllPoints ? cellPts : newCellPts.Get());
      }
    }
    outDS->GetCellData()->CopyData(inCD, cellId, newCellId);
  }

  for (int i = 0; i < nbBlocks; i++)
  {
    blocks[i]->Squeeze();
  }

  this->UpdateProgress(1.);
  return 1;
}

//----------------------------------------------------------------------------
int vtkSplitByCellScalarFilter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkSplitByCellScalarFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Pass All Points: "
     << (this->GetPassAllPoints() ? "On" : "Off") << std::endl;
}
