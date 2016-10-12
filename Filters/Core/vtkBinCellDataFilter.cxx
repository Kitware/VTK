/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBinCellDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBinCellDataFilter.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkCellLocator.h"
#include "vtkDataSet.h"
#include "vtkGenericCell.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <algorithm>
#include <map>
#include <sstream>

vtkStandardNewMacro(vtkBinCellDataFilter);

#define CELL_TOLERANCE_FACTOR_SQR  1e-6

namespace
{
typedef std::map<vtkIdType, vtkIdType> IdMap;
bool CompareIdMapCounts(const IdMap::value_type& p1,
                        const IdMap::value_type& p2)
{
  return p1.second < p2.second;
}

int MostFrequentId(vtkIdType* idList, vtkIdType nIds)
{
  IdMap idMap;
  for (vtkIdType i = 0; i < nIds; i++)
  {
    if (idList[i] != -1)
    {
      idMap[idList[i]]++;
    }
  }
  if (idMap.empty())
  {
    return -1;
  }
  return std::max_element(idMap.begin(), idMap.end(),
                          CompareIdMapCounts)->first;
}

int GetBinId(double value, double* binValues, int nBins)
{
  double* lb = std::lower_bound(binValues, binValues + nBins, value);
  return (lb - binValues);
}
}

//---------------------------------------------------------------------------
// Specify a spatial locator for speeding the search process. By
// default an instance of vtkCellLocator is used.
vtkCxxSetObjectMacro(vtkBinCellDataFilter, CellLocator, vtkCellLocator);

//----------------------------------------------------------------------------
vtkBinCellDataFilter::vtkBinCellDataFilter()
{
  this->BinValues = vtkBinValues::New();
  this->BinValues->GenerateValues(2, VTK_DOUBLE_MIN, VTK_DOUBLE_MAX);

  this->CellLocator = NULL;

  this->StoreNumberOfNonzeroBins = true;
  this->NumberOfNonzeroBinsArrayName = 0;
  this->SetNumberOfNonzeroBinsArrayName("NumberOfNonzeroBins");

  this->SpatialMatch = 0;
  this->SetNumberOfInputPorts(2);

  this->CellOverlapMethod = vtkBinCellDataFilter::CELL_CENTROID;
  this->Tolerance = 1.0;
  this->ComputeTolerance = false;
  this->ArrayComponent = 0;

  // by default process source cell scalars
  this->SetInputArrayToProcess(0,1,0,vtkDataObject::FIELD_ASSOCIATION_CELLS,
                               vtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
vtkBinCellDataFilter::~vtkBinCellDataFilter()
{
  this->BinValues->Delete();
  this->SetCellLocator(NULL);
  this->SetNumberOfNonzeroBinsArrayName(0);
}

//----------------------------------------------------------------------------
void vtkBinCellDataFilter::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
void vtkBinCellDataFilter::SetSourceData(vtkDataObject *input)
{
  this->SetInputData(1, input);
}

//----------------------------------------------------------------------------
vtkDataObject *vtkBinCellDataFilter::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return NULL;
  }

  return this->GetExecutive()->GetInputData(1, 0);
}

//----------------------------------------------------------------------------
int vtkBinCellDataFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *source = vtkDataSet::SafeDownCast(
    sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!source)
  {
    return 0;
  }

  // get the bins
  int numBins = this->GetNumberOfBins();
  double *values = this->GetValues();

  // is there data to process?
  vtkDataArray *sourceScalars = this->GetInputArrayToProcess(0, inputVector);
  if (!sourceScalars)
  {
    return 1;
  }

  // Initialize cell count array.
  vtkNew<vtkIdTypeArray> binnedData;
  binnedData->SetNumberOfComponents(numBins+1);
  binnedData->SetNumberOfTuples(input->GetNumberOfCells());
  {
    std::stringstream s;
    s << "binned_" << sourceScalars->GetName();
    binnedData->SetName(s.str().c_str());
  }

  for (int i = 0; i < numBins + 1; i++)
  {
    binnedData->FillComponent(i, 0);
  }

  // pass point and cell data
  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  double tol2 = (this->ComputeTolerance ? VTK_DOUBLE_MAX :
                 (this->Tolerance * this->Tolerance));

  double weights[VTK_CELL_SIZE];
  vtkIdType inputIds[VTK_CELL_SIZE];

  if (!this->CellLocator)
  {
    this->CreateDefaultLocator();
  }
  this->CellLocator->SetDataSet(input);
  this->CellLocator->BuildLocator();

  vtkNew<vtkGenericCell> sourceCell, inputCell;
  vtkIdType cellId = 0;
  input->GetCell(cellId, inputCell.GetPointer());
  vtkCellIterator *srcIt = source->NewCellIterator();
  double pcoords[3], coords[3];
  int subId;
  // iterate over each cell in the source mesh
  for (srcIt->InitTraversal(); !srcIt->IsDoneWithTraversal();
       srcIt->GoToNextCell())
  {
    if (this->CellOverlapMethod == vtkBinCellDataFilter::CELL_CENTROID)
    {
      // identify the centroid of the source cell
      srcIt->GetCell(sourceCell.GetPointer());
      sourceCell->GetParametricCenter(pcoords);
      sourceCell->EvaluateLocation(subId, pcoords, coords, weights);

      // find the cell that contains xyz and get it
      cellId = this->CellLocator->FindCell(coords, tol2, inputCell.GetPointer(),
                                           pcoords, weights);

      if (this->ComputeTolerance && cellId >= 0)
      {
        // compute a tolerance proportional to the cell length.
        double dist2;
        double closestPoint[3];
        inputCell->EvaluatePosition(coords, closestPoint, subId, pcoords, dist2,
                                    weights);
        if (dist2 > (inputCell->GetLength2() * CELL_TOLERANCE_FACTOR_SQR))
        {
          cellId = -1;
        }
      }
    }
    else
    {
      vtkPoints *points = srcIt->GetPoints();
      for (vtkIdType i = 0; i < points->GetNumberOfPoints(); i++)
      {
        points->GetPoint(i, coords);
        inputIds[i] = this->CellLocator->FindCell(coords, tol2,
                                                  inputCell.GetPointer(),
                                                  pcoords, weights);
      }
      cellId = MostFrequentId(inputIds, points->GetNumberOfPoints());
    }

    // if the source cell centroid is within an input cell, bin the source
    // cell's value and increment the associated bin count.
    if (cellId >= 0)
    {
      double value = sourceScalars->GetComponent(srcIt->GetCellId(),
                                                 this->ArrayComponent);
      int bin = GetBinId(value, values, numBins);
      binnedData->SetTypedComponent(
        cellId, bin, binnedData->GetTypedComponent(cellId, bin) + 1);
    }
  }
  srcIt->Delete();

  // add binned data to the output mesh
  output->GetCellData()->AddArray(binnedData.GetPointer());

  if (this->StoreNumberOfNonzeroBins)
  {
    // Initialize # of nonzero bins array.
    vtkNew<vtkIdTypeArray> numNonzeroBins;
    numNonzeroBins->SetNumberOfComponents(1);
    numNonzeroBins->SetNumberOfTuples(input->GetNumberOfCells());
    numNonzeroBins->SetName(this->NumberOfNonzeroBinsArrayName ?
                            this->NumberOfNonzeroBinsArrayName :
                            "NumberOfNonzeroBins");

    for (vtkIdType i = 0; i < binnedData->GetNumberOfTuples(); i++)
    {
        vtkIdType nBins = 0;
        for (vtkIdType j = 0; j < binnedData->GetNumberOfComponents(); j++)
        {
          if (binnedData->GetTypedComponent(i,j) > 0)
          {
            nBins++;
          }
          numNonzeroBins->SetTypedComponent(i, 0, nBins);
        }
    }
    output->GetCellData()->AddArray(numNonzeroBins.GetPointer());
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkBinCellDataFilter::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  outInfo->CopyEntry(sourceInfo,
                     vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->CopyEntry(sourceInfo,
                     vtkStreamingDemandDrivenPipeline::TIME_RANGE());

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
               6);

  // A variation of the bug fix from John Biddiscombe.
  // Make sure that the scalar type and number of components
  // are propagated from the source not the input.
  if (vtkImageData::HasScalarType(sourceInfo))
  {
    vtkImageData::SetScalarType(vtkImageData::GetScalarType(sourceInfo),
                                outInfo);
  }
  if (vtkImageData::HasNumberOfScalarComponents(sourceInfo))
  {
    vtkImageData::SetNumberOfScalarComponents(
      vtkImageData::GetNumberOfScalarComponents(sourceInfo),
      outInfo);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkBinCellDataFilter::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int usePiece = 0;

  // What ever happened to CopyUpdateExtent in vtkDataObject?
  // Copying both piece and extent could be bad.  Setting the piece
  // of a structured data set will affect the extent.
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (output &&
      (!strcmp(output->GetClassName(), "vtkUnstructuredGrid") ||
       !strcmp(output->GetClassName(), "vtkPolyData")))
  {
    usePiece = 1;
  }

  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  sourceInfo->Remove(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  if (sourceInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
  {
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      sourceInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  }

  if ( ! this->SpatialMatch)
  {
    sourceInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
    sourceInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
    sourceInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  }
  else if (this->SpatialMatch == 1)
  {
    if (usePiece)
    {
      // Request an extra ghost level because the probe
      // gets external values with computation prescision problems.
      // I think the probe should be changed to have an epsilon ...
      sourceInfo->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
        outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
      sourceInfo->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
        outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
      sourceInfo->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
        outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS())+1);
    }
    else
    {
      sourceInfo->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()), 6);
    }
  }

  if (usePiece)
  {
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  }
  else
  {
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()), 6);
  }

  // Use the whole input in all processes, and use the requested update
  // extent of the output to divide up the source.
  if (this->SpatialMatch == 2)
  {
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
    sourceInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
    sourceInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
    sourceInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  }
  return 1;
}

//--------------------------------------------------------------------------
// Method manages creation of locators.
void vtkBinCellDataFilter::CreateDefaultLocator()
{
  this->SetCellLocator(NULL);
  this->CellLocator = vtkCellLocator::New();
  this->CellLocator->Register(this);
  this->CellLocator->Delete();
}

//----------------------------------------------------------------------------
void vtkBinCellDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataObject *source = this->GetSource();

  this->Superclass::PrintSelf(os,indent);
  os << indent << "Source: " << source << "\n";
  os << indent << "SpatialMatch: " << ( this->SpatialMatch ? "On" : "Off" )
     << "\n";
}
