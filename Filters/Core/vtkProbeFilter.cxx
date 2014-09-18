/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProbeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProbeFilter.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vector>

vtkStandardNewMacro(vtkProbeFilter);

class vtkProbeFilter::vtkVectorOfArrays :
  public std::vector<vtkDataArray*>
{
};

//----------------------------------------------------------------------------
vtkProbeFilter::vtkProbeFilter()
{
  this->SpatialMatch = 0;
  this->ValidPoints = vtkIdTypeArray::New();
  this->MaskPoints = vtkCharArray::New();
  this->MaskPoints->SetNumberOfComponents(1);
  this->SetNumberOfInputPorts(2);
  this->ValidPointMaskArrayName = 0;
  this->SetValidPointMaskArrayName("vtkValidPointMask");
  this->CellArrays = new vtkVectorOfArrays();
  this->NumberOfValidPoints = 0;

  this->PointList = 0;
  this->CellList = 0;

  this->UseNullPoint = true;

  this->PassCellArrays = 0;
  this->PassPointArrays = 0;
  this->PassFieldArrays = 1;
  this->Tolerance = 1.0;
  this->ComputeTolerance = 1;
}

//----------------------------------------------------------------------------
vtkProbeFilter::~vtkProbeFilter()
{
  this->MaskPoints->Delete();
  this->MaskPoints = 0;
  this->ValidPoints->Delete();
  this->ValidPoints = NULL;
  this->SetValidPointMaskArrayName(0);
  delete this->CellArrays;

  delete this->PointList;
  delete this->CellList;
}

//----------------------------------------------------------------------------
void vtkProbeFilter::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
void vtkProbeFilter::SetSourceData(vtkDataObject *input)
{
  this->SetInputData(1, input);
}

//----------------------------------------------------------------------------
vtkDataObject *vtkProbeFilter::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
    {
    return NULL;
    }

  return this->GetExecutive()->GetInputData(1, 0);
}

//----------------------------------------------------------------------------
int vtkProbeFilter::RequestData(
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

  this->Probe(input, source, output);

  this->PassAttributeData(input, source, output);
  return 1;
}

//----------------------------------------------------------------------------
void vtkProbeFilter::PassAttributeData(
  vtkDataSet* input, vtkDataObject* vtkNotUsed(source), vtkDataSet* output)
{
  // copy point data arrays
  if (this->PassPointArrays)
    {
    int numPtArrays = input->GetPointData()->GetNumberOfArrays();
    for (int i=0; i<numPtArrays; ++i)
      {
      output->GetPointData()->AddArray(input->GetPointData()->GetArray(i));
      }
    }

  // copy cell data arrays
  if (this->PassCellArrays)
    {
    int numCellArrays = input->GetCellData()->GetNumberOfArrays();
    for (int i=0; i<numCellArrays; ++i)
      {
      output->GetCellData()->AddArray(input->GetCellData()->GetArray(i));
      }
    }

  if (this->PassFieldArrays)
    {
    // nothing to do, vtkDemandDrivenPipeline takes care of that.
    }
  else
    {
    output->GetFieldData()->Initialize();
    }
}

//----------------------------------------------------------------------------
void vtkProbeFilter::BuildFieldList(vtkDataSet* source)
{
  delete this->PointList;
  delete this->CellList;

  this->PointList = new vtkDataSetAttributes::FieldList(1);
  this->PointList->InitializeFieldList(source->GetPointData());

  this->CellList = new vtkDataSetAttributes::FieldList(1);
  this->CellList->InitializeFieldList(source->GetCellData());
}

//----------------------------------------------------------------------------
// * input -- dataset probed with
// * source -- dataset probed into
// * output - output.
void vtkProbeFilter::InitializeForProbing(vtkDataSet* input,
  vtkDataSet* output)
{
  if (!this->PointList || !this->CellList)
    {
    vtkErrorMacro("BuildFieldList() must be called before calling this method.");
    return;
    }

  vtkIdType numPts = input->GetNumberOfPoints();

  // Initialize valid points/mask points arrays.
  this->NumberOfValidPoints = 0;
  this->ValidPoints->Allocate(numPts);
  this->MaskPoints->SetNumberOfTuples(numPts);
  this->MaskPoints->FillComponent(0, 0);
  this->MaskPoints->SetName(this->ValidPointMaskArrayName?
    this->ValidPointMaskArrayName: "vtkValidPointMask");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  // Allocate storage for output PointData
  // All input PD is passed to output as PD. Those arrays in input CD that are
  // not present in output PD will be passed as output PD.
  vtkPointData* outPD = output->GetPointData();
  outPD->InterpolateAllocate((*this->PointList), numPts, numPts);

  vtkCellData* tempCellData = vtkCellData::New();
  tempCellData->InterpolateAllocate( (*this->CellList), numPts, numPts);

  this->CellArrays->clear();
  int numCellArrays = tempCellData->GetNumberOfArrays();
  for (int cc=0; cc < numCellArrays; cc++)
    {
    vtkDataArray* inArray = tempCellData->GetArray(cc);
    if (inArray && inArray->GetName() && !outPD->GetArray(inArray->GetName()))
      {
      outPD->AddArray(inArray);
      this->CellArrays->push_back(inArray);
      }
    }
  tempCellData->Delete();

  outPD->AddArray(this->MaskPoints);

  // Since we haven't resize the point arrays, we need to fill them up with
  // nulls whenever we have a miss when probing.
  this->UseNullPoint = true;

}

//----------------------------------------------------------------------------
void vtkProbeFilter::Probe(vtkDataSet *input, vtkDataSet *source,
                           vtkDataSet *output)
{
  this->BuildFieldList(source);
  this->InitializeForProbing(input, output);
  this->ProbeEmptyPoints(input, 0, source, output);
}

//----------------------------------------------------------------------------
void vtkProbeFilter::ProbeEmptyPoints(vtkDataSet *input,
  int srcIdx,
  vtkDataSet *source, vtkDataSet *output)
{
  vtkIdType ptId, numPts;
  double x[3], tol2;
  vtkCell *cell;
  vtkPointData *pd, *outPD;
  vtkCellData* cd;
  int subId;
  double pcoords[3], *weights;
  double fastweights[256];

  vtkDebugMacro(<<"Probing data");

  pd = source->GetPointData();
  cd = source->GetCellData();

  // lets use a stack allocated array if possible for performance reasons
  int mcs = source->GetMaxCellSize();
  if (mcs<=256)
    {
    weights = fastweights;
    }
  else
    {
    weights = new double[mcs];
    }

  numPts = input->GetNumberOfPoints();
  outPD = output->GetPointData();

  char* maskArray = this->MaskPoints->GetPointer(0);

  if (this->ComputeTolerance)
    {
    // Use tolerance as a function of size of source data
    //
    tol2 = source->GetLength();
    tol2 = tol2 ? tol2*tol2 / 1000.0 : 0.001;

    // the actual sampling rate needs to be considered for a
    // more appropriate / accurate selection of the tolerance.
    // Otherwise the tolerance simply determined above might be
    // so large as to cause incorrect cell location
    double bounds[6];
    source->GetBounds(bounds);
    double minRes = 10000000000.0;
    double axisRes[3];
    for ( int  i = 0;  i < 3;  i ++ )
      {
      axisRes[i] = ( bounds[i * 2 + 1] - bounds[i * 2] ) / numPts;
      if ( (axisRes[i] > 0.0) && (axisRes[i] < minRes) )
        minRes = axisRes[i];
      }
    double minRes2 = minRes * minRes;
    tol2 = tol2 > minRes2 ? minRes2 : tol2;

    // Don't go below epsilon for a double
    tol2 = (tol2 < VTK_DBL_EPSILON) ? VTK_DBL_EPSILON : tol2;
    }
  else
    {
    tol2 = this->Tolerance * this->Tolerance;
    }

  // Loop over all input points, interpolating source data
  //
  int abort=0;
  vtkIdType progressInterval=numPts/20 + 1;
  for (ptId=0; ptId < numPts && !abort; ptId++)
    {
    if ( !(ptId % progressInterval) )
      {
      this->UpdateProgress(static_cast<double>(ptId)/numPts);
      abort = GetAbortExecute();
      }

    if (maskArray[ptId] == static_cast<char>(1))
      {
      // skip points which have already been probed with success.
      // This is helpful for multiblock dataset probing.
      continue;
      }

    // Get the xyz coordinate of the point in the input dataset
    input->GetPoint(ptId, x);

    // Find the cell that contains xyz and get it
    vtkIdType cellId = source->FindCell(x,NULL,-1,tol2,subId,pcoords,weights);
    if (cellId >= 0)
      {
      cell = source->GetCell(cellId);
      // If we found a cell, let's make sure that the point is within
      // a certain size of the cell when it is slightly outside.
      // The tolerance check above is based on the bounds of the whole
      // dataset which may be significantly larger than the cell. When
      // that happens, even a small tolerance may lead to finding a cell
      // when the point is significantly outside that cell. This check
      // is based on the cell's size. The tolerance here is significantly
      // larger, 1/10 the size of the cell.
      double dist2;
      double closestPoint[3];
      cell->EvaluatePosition(x, closestPoint, subId,
                             pcoords, dist2, weights);
      if (dist2 > cell->GetLength2() * 0.01)
        {
        cell = 0;
        }
      }
    else
      {
      cell = 0;
      }
    if (cell)
      {
      // Interpolate the point data
      outPD->InterpolatePoint((*this->PointList), pd, srcIdx, ptId,
        cell->PointIds, weights);
      this->ValidPoints->InsertNextValue(ptId);
      this->NumberOfValidPoints++;
      vtkVectorOfArrays::iterator iter;
      for (iter = this->CellArrays->begin(); iter != this->CellArrays->end();
        ++iter)
        {
        vtkDataArray* inArray = cd->GetArray((*iter)->GetName());
        if (inArray)
          {
          outPD->CopyTuple(inArray, *iter, cellId, ptId);
          }
        }
      maskArray[ptId] = static_cast<char>(1);
      }
    else
      {
      if (this->UseNullPoint)
        {
        outPD->NullPoint(ptId);
        }
      }
    }

  if (mcs>256)
    {
    delete [] weights;
    }
}

//----------------------------------------------------------------------------
int vtkProbeFilter::RequestInformation(
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
int vtkProbeFilter::RequestUpdateExtent(
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

//----------------------------------------------------------------------------
void vtkProbeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataObject *source = this->GetSource();

  this->Superclass::PrintSelf(os,indent);
  os << indent << "Source: " << source << "\n";
  os << indent << "SpatialMatch: " << ( this->SpatialMatch ? "On" : "Off" ) << "\n";
  os << indent << "ValidPointMaskArrayName: " << (this->ValidPointMaskArrayName?
    this->ValidPointMaskArrayName : "vtkValidPointMask") << "\n";
  os << indent << "ValidPoints: " << this->ValidPoints << "\n";
  os << indent << "PassFieldArrays: "
     << (this->PassFieldArrays? "On" : " Off") << "\n";
}
