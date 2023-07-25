// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkExtractGeometry.h"

#include "vtk3DLinearGridCrinkleExtractor.h"
#include "vtkDoubleArray.h"
#include "vtkEventForwarderCommand.h"
#include "vtkExtractCells.h"
#include "vtkIdList.h"
#include "vtkImplicitFunction.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkExtractGeometry);
vtkCxxSetObjectMacro(vtkExtractGeometry, ImplicitFunction, vtkImplicitFunction);

//------------------------------------------------------------------------------
// Construct object with ExtractInside turned on.
vtkExtractGeometry::vtkExtractGeometry(vtkImplicitFunction* f)
{
  this->ImplicitFunction = f;
  if (this->ImplicitFunction)
  {
    this->ImplicitFunction->Register(this);
  }

  this->ExtractInside = 1;
  this->ExtractBoundaryCells = 0;
  this->ExtractOnlyBoundaryCells = 0;
}

//------------------------------------------------------------------------------
vtkExtractGeometry::~vtkExtractGeometry()
{
  this->SetImplicitFunction(nullptr);
}

//------------------------------------------------------------------------------
// Overload standard modified time function. If implicit function is modified,
// then this object is modified as well.
vtkMTimeType vtkExtractGeometry::GetMTime()
{
  vtkMTimeType mTime = this->MTime.GetMTime();
  vtkMTimeType impFuncMTime;

  if (this->ImplicitFunction != nullptr)
  {
    impFuncMTime = this->ImplicitFunction->GetMTime();
    mTime = (impFuncMTime > mTime ? impFuncMTime : mTime);
  }

  return mTime;
}

namespace
{
//------------------------------------------------------------------------------
struct EvaluateCells
{
  vtkExtractGeometry* Self;
  vtkDataSet* Input;
  vtkUnsignedCharArray* InsidenessPointsArray;
  vtkDoubleArray* ScalarsArray;
  vtkIdType NumberOfCells;
  vtkIdList* KeptCellsList;

  vtkSMPThreadLocal<vtkSmartPointer<vtkIdList>> TLCellIds;
  vtkNew<vtkUnsignedCharArray> InsidenessCellsArray;

  EvaluateCells(vtkExtractGeometry* self, vtkDataSet* input,
    vtkUnsignedCharArray* insidenessPointsArray, vtkDoubleArray* scalarsArray,
    vtkIdList* keptCellsList)
    : Self(self)
    , Input(input)
    , InsidenessPointsArray(insidenessPointsArray)
    , ScalarsArray(scalarsArray)
    , NumberOfCells(input->GetNumberOfCells())
    , KeptCellsList(keptCellsList)
  {
    this->InsidenessCellsArray->SetNumberOfValues(input->GetNumberOfCells());
    if (this->NumberOfCells > 0)
    {
      // ensure that internal structures are initialized.
      this->Input->GetCell(0);
    }
  }

  void Initialize() { this->TLCellIds.Local().TakeReference(vtkIdList::New()); }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    const bool isFirst = vtkSMPTools::GetSingleThread();

    auto scalars = vtk::DataArrayValueRange<1>(this->ScalarsArray);
    auto insidePoints = vtk::DataArrayValueRange<1>(this->InsidenessPointsArray);
    auto insideCells = vtk::DataArrayValueRange<1>(this->InsidenessCellsArray);

    auto cellIds = this->TLCellIds.Local();
    vtkIdType numCellPts;
    const vtkIdType* cellPts;
    vtkIdType checkAbortInterval = std::min((end - begin) / 10 + 1, (vtkIdType)1000);
    for (vtkIdType cellId = begin; cellId < end; ++cellId)
    {
      if (cellId % checkAbortInterval == 0)
      {
        if (isFirst)
        {
          this->Self->CheckAbort();
        }
        if (this->Self->GetAbortOutput())
        {
          break;
        }
      }

      this->Input->GetCellPoints(cellId, numCellPts, cellPts, cellIds);
      if (!this->Self->GetExtractBoundaryCells()) // don't want boundary cells
      {
        bool allInside = true;
        for (vtkIdType i = 0; i < numCellPts; ++i)
        {
          if (insidePoints[cellPts[i]] == 0)
          {
            allInside = false;
            break;
          }
        }
        insideCells[cellId] = static_cast<unsigned char>(allInside);
      }
      else // want boundary cells
      {
        bool inside = false;
        bool outside = false;
        for (vtkIdType i = 0; i < numCellPts; ++i)
        {
          if (scalars[cellPts[i]] <= 0.0)
          {
            inside = true;
          }
          else
          {
            outside = true;
          }
        }
        if (this->Self->GetExtractOnlyBoundaryCells())
        {
          insideCells[cellId] = static_cast<unsigned char>(inside && outside);
        }
        else
        {
          insideCells[cellId] = static_cast<unsigned char>(inside);
        }
      } // if mapping boundary cells
    }
    if (isFirst)
    {
      this->Self->UpdateProgress(0.5 + end * 0.5 / this->NumberOfCells);
    }
  }

  void Reduce()
  {
    this->KeptCellsList->Allocate(this->NumberOfCells);
    for (vtkIdType cellId = 0; cellId < this->NumberOfCells; ++cellId)
    {
      if (this->InsidenessCellsArray->GetValue(cellId))
      {
        this->KeptCellsList->InsertNextId(cellId);
      }
    }
  }
};
} // end anon namespace

//------------------------------------------------------------------------------
int vtkExtractGeometry::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the input
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0]->GetInformationObject(0));

  if (!this->GetExtractInside() && this->GetExtractOnlyBoundaryCells() &&
    this->GetExtractBoundaryCells() &&
    vtk3DLinearGridCrinkleExtractor::CanFullyProcessDataObject(input))
  {
    vtkNew<vtk3DLinearGridCrinkleExtractor> linear3DExtractor;
    linear3DExtractor->SetImplicitFunction(this->GetImplicitFunction());
    linear3DExtractor->SetCopyPointData(true);
    linear3DExtractor->SetCopyCellData(true);

    vtkNew<vtkEventForwarderCommand> progressForwarder;
    progressForwarder->SetTarget(this);
    linear3DExtractor->AddObserver(vtkCommand::ProgressEvent, progressForwarder);

    return linear3DExtractor->ProcessRequest(request, inputVector, outputVector);
  }

  vtkDebugMacro(<< "Extracting geometry");

  if (!this->ImplicitFunction)
  {
    vtkErrorMacro(<< "No implicit function specified");
    return 1;
  }

  const double multiplier = this->ExtractInside ? 1.0 : -1.0;

  vtkNew<vtkUnsignedCharArray> insidenessArray;
  vtkNew<vtkDoubleArray> scalarArray;
  // call that to guarantee thread safety
  this->ImplicitFunction->EvaluateFunction(0, 0, 0);
  if (!this->ExtractBoundaryCells)
  {
    insidenessArray->SetNumberOfValues(input->GetNumberOfPoints());
    vtkSMPTools::For(0, input->GetNumberOfPoints(), [&](vtkIdType begin, vtkIdType end) {
      double x[3];
      for (vtkIdType ptId = begin; ptId < end; ++ptId)
      {
        input->GetPoint(ptId, x);
        insidenessArray->SetValue(ptId,
          static_cast<unsigned char>(this->ImplicitFunction->FunctionValue(x) * multiplier < 0.0));
      }
    });
  }
  else
  {
    scalarArray->SetNumberOfValues(input->GetNumberOfPoints());
    vtkSMPTools::For(0, input->GetNumberOfPoints(), [&](vtkIdType begin, vtkIdType end) {
      double x[3];
      for (vtkIdType ptId = begin; ptId < end; ++ptId)
      {
        input->GetPoint(ptId, x);
        scalarArray->SetValue(ptId, this->ImplicitFunction->FunctionValue(x) * multiplier);
      }
    });
  }
  this->UpdateProgress(0.5);

  vtkNew<vtkIdList> keptCellsList;
  EvaluateCells evaluateCells(this, input, insidenessArray, scalarArray, keptCellsList);
  vtkSMPTools::For(0, input->GetNumberOfCells(), evaluateCells);
  if (this->CheckAbort())
  {
    return 1;
  }

  // call vtkExtractCells
  vtkNew<vtkExtractCells> extractCells;
  extractCells->SetContainerAlgorithm(this);
  extractCells->SetInputData(input);
  extractCells->SetExtractAllCells(input->GetNumberOfCells() == keptCellsList->GetNumberOfIds());
  if (!extractCells->GetExtractAllCells())
  {
    extractCells->SetCellList(keptCellsList);
  }
  extractCells->AssumeSortedAndUniqueIdsOn();
  extractCells->PassThroughCellIdsOff();

  vtkNew<vtkEventForwarderCommand> progressForwarder;
  progressForwarder->SetTarget(this);
  extractCells->AddObserver(vtkCommand::ProgressEvent, progressForwarder);

  return extractCells->ProcessRequest(request, inputVector, outputVector);
}

//------------------------------------------------------------------------------
int vtkExtractGeometry::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
void vtkExtractGeometry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Implicit Function: " << static_cast<void*>(this->ImplicitFunction) << "\n";
  os << indent << "Extract Inside: " << (this->ExtractInside ? "On\n" : "Off\n");
  os << indent << "Extract Boundary Cells: " << (this->ExtractBoundaryCells ? "On\n" : "Off\n");
  os << indent
     << "Extract Only Boundary Cells: " << (this->ExtractOnlyBoundaryCells ? "On\n" : "Off\n");
}
VTK_ABI_NAMESPACE_END
