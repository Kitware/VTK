/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAppendPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAppendPolyData.h"

#include "vtkAlgorithmOutput.h"
#include "vtkArrayDispatch.h"
#include "vtkAssume.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTrivialProducer.h"

#include <cassert>
#include <cstdlib>

vtkStandardNewMacro(vtkAppendPolyData);

//----------------------------------------------------------------------------
vtkAppendPolyData::vtkAppendPolyData()
{
  this->ParallelStreaming = 0;
  this->UserManagedInputs = 0;
  this->OutputPointsPrecision = vtkAlgorithm::DEFAULT_PRECISION;
}

//----------------------------------------------------------------------------
vtkAppendPolyData::~vtkAppendPolyData() = default;

//----------------------------------------------------------------------------
// Add a dataset to the list of data to append.
void vtkAppendPolyData::AddInputData(vtkPolyData* ds)
{
  if (this->UserManagedInputs)
  {
    vtkErrorMacro(<< "AddInput is not supported if UserManagedInputs is true");
    return;
  }
  this->Superclass::AddInputData(ds);
}

//----------------------------------------------------------------------------
// Remove a dataset from the list of data to append.
void vtkAppendPolyData::RemoveInputData(vtkPolyData* ds)
{
  if (this->UserManagedInputs)
  {
    vtkErrorMacro(<< "RemoveInput is not supported if UserManagedInputs is true");
    return;
  }

  if (!ds)
  {
    return;
  }
  int numCons = this->GetNumberOfInputConnections(0);
  for (int i = 0; i < numCons; i++)
  {
    if (this->GetInput(i) == ds)
    {
      this->RemoveInputConnection(0, this->GetInputConnection(0, i));
    }
  }
}

//----------------------------------------------------------------------------
// make ProcessObject function visible
// should only be used when UserManagedInputs is true.
void vtkAppendPolyData::SetNumberOfInputs(int num)
{
  if (!this->UserManagedInputs)
  {
    vtkErrorMacro(<< "SetNumberOfInputs is not supported if UserManagedInputs is false");
    return;
  }

  // Ask the superclass to set the number of connections.
  this->SetNumberOfInputConnections(0, num);
}

//----------------------------------------------------------------------------
void vtkAppendPolyData::SetInputDataByNumber(int num, vtkPolyData* input)
{
  vtkTrivialProducer* tp = vtkTrivialProducer::New();
  tp->SetOutput(input);
  this->SetInputConnectionByNumber(num, tp->GetOutputPort());
  tp->Delete();
}

//----------------------------------------------------------------------------
// Set Nth input, should only be used when UserManagedInputs is true.
void vtkAppendPolyData::SetInputConnectionByNumber(int num, vtkAlgorithmOutput* input)
{
  if (!this->UserManagedInputs)
  {
    vtkErrorMacro(<< "SetInputConnectionByNumber is not supported if UserManagedInputs is false");
    return;
  }

  // Ask the superclass to connect the input.
  this->SetNthInputConnection(0, num, input);
}

//----------------------------------------------------------------------------
int vtkAppendPolyData::ExecuteAppend(vtkPolyData* output, vtkPolyData* inputs[], int numInputs)
{
  int idx;
  vtkPolyData* ds;
  vtkPoints* inPts;
  vtkPoints* newPts;
  vtkCellArray *inVerts, *newVerts;
  vtkCellArray *inLines, *newLines;
  vtkCellArray *inPolys, *newPolys;
  vtkIdType sizePolys, numPolys;
  vtkCellArray *inStrips, *newStrips;
  vtkIdType numPts, numCells;
  vtkPointData* inPD = nullptr;
  vtkCellData* inCD = nullptr;
  vtkPointData* outputPD = output->GetPointData();
  vtkCellData* outputCD = output->GetCellData();

  vtkDebugMacro(<< "Appending polydata");

  // loop over all data sets, checking to see what point data is available.
  numPts = 0;
  numCells = 0;
  sizePolys = numPolys = 0;

  int countPD = 0;
  int countCD = 0;

  vtkIdType numVerts = 0, numLines = 0, numStrips = 0;
  vtkIdType sizeLines = 0, sizeStrips = 0, sizeVerts = 0;

  // These Field lists are very picky.  Count the number of non empty inputs
  // so we can initialize them properly.
  for (idx = 0; idx < numInputs; ++idx)
  {
    ds = inputs[idx];
    if (ds != nullptr)
    {
      if (ds->GetNumberOfPoints() > 0)
      {
        ++countPD;
      }
      if (ds->GetNumberOfCells() > 0)
      {
        ++countCD;
      } // for a data set that has cells
    }   // for a non nullptr input
  }     // for each input

  // These are used to determine which fields are available for appending
  vtkDataSetAttributes::FieldList ptList(countPD);
  vtkDataSetAttributes::FieldList cellList(countCD);

  countPD = countCD = 0;
  for (idx = 0; idx < numInputs; ++idx)
  {
    ds = inputs[idx];
    if (ds != nullptr)
    {
      // Skip points and cells if there are no points.  Empty inputs may have no arrays.
      if (ds->GetNumberOfPoints() > 0)
      {
        numPts += ds->GetNumberOfPoints();
        // Take intersection of available point data fields.
        inPD = ds->GetPointData();
        if (countPD == 0)
        {
          ptList.InitializeFieldList(inPD);
        }
        else
        {
          ptList.IntersectFieldList(inPD);
        }
        ++countPD;
      } // for a data set that has points

      // Although we cannot have cells without points ... let's not nest.
      if (ds->GetNumberOfCells() > 0)
      {
        if (ds->GetVerts())
        {
          sizeVerts += ds->GetVerts()->GetNumberOfConnectivityIds();
        }
        if (ds->GetLines())
        {
          sizeLines += ds->GetLines()->GetNumberOfConnectivityIds();
        }
        // keep track of the size of the poly cell array
        if (ds->GetPolys())
        {
          sizePolys += ds->GetPolys()->GetNumberOfConnectivityIds();
        }
        if (ds->GetStrips())
        {
          sizeStrips += ds->GetStrips()->GetNumberOfConnectivityIds();
        }

        numCells += ds->GetNumberOfCells();
        // Count the cells of each type.
        // This is used to ensure that cell data is copied at the correct
        // locations in the output.
        numVerts += ds->GetNumberOfVerts();
        numLines += ds->GetNumberOfLines();
        numPolys += ds->GetNumberOfPolys();
        numStrips += ds->GetNumberOfStrips();

        inCD = ds->GetCellData();
        if (countCD == 0)
        {
          cellList.InitializeFieldList(inCD);
        }
        else
        {
          cellList.IntersectFieldList(inCD);
        }
        ++countCD;
      } // for a data set that has cells
    }   // for a non nullptr input
  }     // for each input

  if (numPts < 1 && numCells < 1)
  {
    vtkDebugMacro(<< "No data to append!");
    return 1;
  }
  this->UpdateProgress(0.10);

  // Examine the points and check if they're the same type. If not,
  // use highest (double probably), otherwise the type of the first
  // array (float no doubt). Depends on defs in vtkSetGet.h - Warning.
  int ttype, firstType = 1;
  int pointtype = 0;

  // Keep track of types for fast point append
  for (idx = 0; idx < numInputs; ++idx)
  {
    ds = inputs[idx];
    if (ds != nullptr && ds->GetNumberOfPoints() > 0)
    {
      if (firstType)
      {
        firstType = 0;
        pointtype = ds->GetPoints()->GetData()->GetDataType();
      }
      ttype = ds->GetPoints()->GetData()->GetDataType();
      pointtype = pointtype > ttype ? pointtype : ttype;
    }
  }

  // Allocate geometry/topology
  newPts = vtkPoints::New();

  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    newPts->SetDataType(pointtype);
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(VTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(VTK_DOUBLE);
  }

  newPts->SetNumberOfPoints(numPts);

  newVerts = vtkCellArray::New();
  bool allocated = newVerts->AllocateExact(numVerts, sizeVerts);

  if (sizeVerts > 0 && !allocated)
  {
    vtkErrorMacro(<< "Memory allocation failed in append filter");
    return 0;
  }

  newLines = vtkCellArray::New();
  allocated = newLines->AllocateExact(numLines, sizeLines);

  if (sizeLines > 0 && !allocated)
  {
    vtkErrorMacro(<< "Memory allocation failed in append filter");
    return 0;
  }

  newPolys = vtkCellArray::New();
  allocated = newPolys->AllocateExact(numPolys, sizePolys);

  if (sizePolys > 0 && !allocated)
  {
    vtkErrorMacro(<< "Memory allocation failed in append filter");
    return 0;
  }

  newStrips = vtkCellArray::New();
  allocated = newStrips->AllocateExact(numStrips, sizeStrips);

  if (sizeStrips > 0 && !allocated)
  {
    vtkErrorMacro(<< "Memory allocation failed in append filter");
    return 0;
  }

  // Since points are cells are not merged,
  // this filter can easily pass all field arrays, including global ids.
  outputPD->CopyAllOn(vtkDataSetAttributes::COPYTUPLE);
  outputCD->CopyAllOn(vtkDataSetAttributes::COPYTUPLE);

  // Allocate the point and cell data
  outputPD->CopyAllocate(ptList, numPts);
  outputCD->CopyAllocate(cellList, numCells);

  // loop over all input sets
  vtkIdType ptOffset = 0;
  vtkIdType vertOffset = 0;
  vtkIdType linesOffset = numVerts;
  vtkIdType polysOffset = numVerts + numLines;
  vtkIdType stripsOffset = numVerts + numLines + numPolys;
  countPD = countCD = 0;
  for (idx = 0; idx < numInputs; ++idx)
  {
    this->UpdateProgress(0.2 + 0.8 * idx / numInputs);
    ds = inputs[idx];
    // this check is not necessary, but I'll put it in anyway
    if (ds != nullptr)
    {
      numPts = ds->GetNumberOfPoints();
      numCells = ds->GetNumberOfCells();
      if (numPts <= 0 && numCells <= 0)
      {
        continue; // no input, just skip
      }

      inPD = ds->GetPointData();
      inCD = ds->GetCellData();

      inPts = ds->GetPoints();
      inVerts = ds->GetVerts();
      inLines = ds->GetLines();
      inPolys = ds->GetPolys();
      inStrips = ds->GetStrips();

      if (ds->GetNumberOfPoints() > 0)
      {
        // copy points directly
        this->AppendData(newPts->GetData(), inPts->GetData(), ptOffset);
        outputPD->CopyData(ptList, inPD, countPD, ptOffset, numPts, 0);
        ++countPD;
      }

      if (ds->GetNumberOfCells() > 0)
      {
        // These are the cellIDs at which each of the cell types start.
        vtkIdType vertsIndex = 0;
        vtkIdType linesIndex = ds->GetNumberOfVerts();
        vtkIdType polysIndex = linesIndex + ds->GetNumberOfLines();
        vtkIdType stripsIndex = polysIndex + ds->GetNumberOfPolys();

        // copy the cells
        this->AppendCells(newVerts, inVerts, ptOffset);
        this->AppendCells(newLines, inLines, ptOffset);
        this->AppendCells(newPolys, inPolys, ptOffset);
        this->AppendCells(newStrips, inStrips, ptOffset);

        // copy cell data
        outputCD->CopyData(cellList, inCD, countCD, vertOffset, ds->GetNumberOfVerts(), vertsIndex);
        vertOffset += ds->GetNumberOfVerts();
        outputCD->CopyData(
          cellList, inCD, countCD, linesOffset, ds->GetNumberOfLines(), linesIndex);
        linesOffset += ds->GetNumberOfLines();
        outputCD->CopyData(
          cellList, inCD, countCD, polysOffset, ds->GetNumberOfPolys(), polysIndex);
        polysOffset += ds->GetNumberOfPolys();
        outputCD->CopyData(
          cellList, inCD, countCD, stripsOffset, ds->GetNumberOfStrips(), stripsIndex);
        stripsOffset += ds->GetNumberOfStrips();
        ++countCD;
      }
      ptOffset += numPts;
    }
  }

  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();

  if (newVerts->GetNumberOfCells() > 0)
  {
    output->SetVerts(newVerts);
  }
  newVerts->Delete();

  if (newLines->GetNumberOfCells() > 0)
  {
    output->SetLines(newLines);
  }
  newLines->Delete();

  if (newPolys->GetNumberOfCells() > 0)
  {
    output->SetPolys(newPolys);
  }
  newPolys->Delete();

  if (newStrips->GetNumberOfCells() > 0)
  {
    output->SetStrips(newStrips);
  }
  newStrips->Delete();

  // When all optimizations are complete, this squeeze will be unnecessary.
  // (But it does not seem to cost much.)
  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
// This method is much too long, and has to be broken up!
// Append data sets into single polygonal data set.
int vtkAppendPolyData::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info object
  // get the output
  vtkPolyData* output = vtkPolyData::GetData(outputVector, 0);

  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  if (numInputs == 1)
  {
    output->ShallowCopy(vtkPolyData::GetData(inputVector[0], 0));
    return 1;
  }

  vtkPolyData** inputs = new vtkPolyData*[numInputs];
  for (int idx = 0; idx < numInputs; ++idx)
  {
    inputs[idx] = vtkPolyData::GetData(inputVector[0], idx);
  }
  int retVal = this->ExecuteAppend(output, inputs, numInputs);
  delete[] inputs;
  return retVal;
}

//----------------------------------------------------------------------------
int vtkAppendPolyData::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the output info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int piece, numPieces, ghostLevel;
  int idx;

  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevel = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  // make sure piece is valid
  if (piece < 0 || piece >= numPieces)
  {
    return 0;
  }

  int numInputs = this->GetNumberOfInputConnections(0);
  if (this->ParallelStreaming)
  {
    piece = piece * numInputs;
    numPieces = numPieces * numInputs;
  }

  vtkInformation* inInfo;
  // just copy the Update extent as default behavior.
  for (idx = 0; idx < numInputs; ++idx)
  {
    inInfo = inputVector[0]->GetInformationObject(idx);
    if (inInfo)
    {
      if (this->ParallelStreaming)
      {
        inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece + idx);
        inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
        inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevel);
      }
      else
      {
        inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
        inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
        inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevel);
      }
    }
  }
  // Let downstream request a subset of connection 0, for connections >= 1
  // send their WHOLE_EXTENT as UPDATE_EXTENT.
  for (idx = 1; idx < numInputs; ++idx)
  {
    vtkInformation* inputInfo = inputVector[0]->GetInformationObject(idx);
    if (inputInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
    {
      int ext[6];
      inputInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext);
      inputInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ext, 6);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
vtkPolyData* vtkAppendPolyData::GetInput(int idx)
{
  return vtkPolyData::SafeDownCast(this->GetExecutive()->GetInputData(0, idx));
}

//----------------------------------------------------------------------------
void vtkAppendPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << "ParallelStreaming:" << (this->ParallelStreaming ? "On" : "Off") << endl;
  os << "UserManagedInputs:" << (this->UserManagedInputs ? "On" : "Off") << endl;
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << endl;
}

//----------------------------------------------------------------------------
namespace
{
struct AppendDataWorker
{
  vtkIdType Offset;

  AppendDataWorker(vtkIdType offset)
    : Offset(offset)
  {
  }

  template <typename Array1T, typename Array2T>
  void operator()(Array1T* dest, Array2T* src)
  {
    VTK_ASSUME(src->GetNumberOfComponents() == dest->GetNumberOfComponents());
    const auto srcTuples = vtk::DataArrayTupleRange(src);

    // Offset the dstTuple range to begin at this->Offset
    auto dstTuples = vtk::DataArrayTupleRange(dest, this->Offset);

    std::copy(srcTuples.cbegin(), srcTuples.cend(), dstTuples.begin());
  }
};
} // end anon namespace

//----------------------------------------------------------------------------
void vtkAppendPolyData::AppendData(vtkDataArray* dest, vtkDataArray* src, vtkIdType offset)
{
  assert("Arrays have same number of components." &&
    src->GetNumberOfComponents() == dest->GetNumberOfComponents());
  assert("Destination array has enough tuples." &&
    src->GetNumberOfTuples() + offset <= dest->GetNumberOfTuples());

  AppendDataWorker worker(offset);
  if (!vtkArrayDispatch::Dispatch2SameValueType::Execute(dest, src, worker))
  {
    // Use vtkDataArray API when fast-path dispatch fails.
    worker(dest, src);
  }
}

//----------------------------------------------------------------------------
void vtkAppendPolyData::AppendCells(vtkCellArray* dst, vtkCellArray* src, vtkIdType offset)
{
  dst->Append(src, offset);
}

//----------------------------------------------------------------------------
int vtkAppendPolyData::FillInputPortInformation(int port, vtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return 1;
}
