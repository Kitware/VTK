// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Hide VTK_DEPRECATED_IN_9_3_0() warnings.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkThreshold.h"

#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkEventForwarderCommand.h"
#include "vtkExtractCells.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <limits>

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkThreshold);

//------------------------------------------------------------------------------
// Construct with lower threshold=0, upper threshold=1, and threshold
// function=upper AllScalars=1.
vtkThreshold::vtkThreshold()
{
  this->LowerThreshold = -std::numeric_limits<double>::infinity();
  this->UpperThreshold = std::numeric_limits<double>::infinity();

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, vtkDataSetAttributes::SCALARS);
}

vtkThreshold::~vtkThreshold() = default;

//------------------------------------------------------------------------------
int vtkThreshold::Lower(double s) const
{
  return (s <= this->LowerThreshold ? 1 : 0);
}

//------------------------------------------------------------------------------
int vtkThreshold::Upper(double s) const
{
  return (s >= this->UpperThreshold ? 1 : 0);
}

//------------------------------------------------------------------------------
int vtkThreshold::Between(double s) const
{
  return (s >= this->LowerThreshold ? (s <= this->UpperThreshold ? 1 : 0) : 0);
}

//------------------------------------------------------------------------------
void vtkThreshold::SetThresholdFunction(int function)
{
  if (this->GetThresholdFunction() != function)
  {
    switch (function)
    {
      case vtkThreshold::THRESHOLD_BETWEEN:
        this->ThresholdFunction = &vtkThreshold::Between;
        break;
      case vtkThreshold::THRESHOLD_LOWER:
        this->ThresholdFunction = &vtkThreshold::Lower;
        break;
      case vtkThreshold::THRESHOLD_UPPER:
        this->ThresholdFunction = &vtkThreshold::Upper;
        break;
    }

    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkThreshold::GetThresholdFunction()
{
  if (this->ThresholdFunction == &vtkThreshold::Between)
  {
    return vtkThreshold::THRESHOLD_BETWEEN;
  }
  else if (this->ThresholdFunction == &vtkThreshold::Lower)
  {
    return vtkThreshold::THRESHOLD_LOWER;
  }
  else if (this->ThresholdFunction == &vtkThreshold::Upper)
  {
    return vtkThreshold::THRESHOLD_UPPER;
  }

  // Added to avoid warning. Should never be reached.
  return -1;
}

//------------------------------------------------------------------------------
template <typename TScalarArray>
struct vtkThreshold::EvaluateCellsFunctor
{
  vtkThreshold* Self;
  vtkDataSet* Input;
  TScalarArray* ScalarsArray;
  vtkUnsignedCharArray* GhostArray;
  bool UsePointScalars;
  vtkIdType NumberOfCells;

  vtkSMPThreadLocal<vtkSmartPointer<vtkIdList>> TLCellIds;

  vtkNew<vtkUnsignedCharArray> InsidenessArray;
  vtkIdList* KeptCellsList;

  EvaluateCellsFunctor(vtkThreshold* self, vtkDataSet* input, TScalarArray* scalarsArray,
    vtkUnsignedCharArray* ghostArray, bool usePointScalars, vtkIdList* keptCellsList)
    : Self(self)
    , Input(input)
    , ScalarsArray(scalarsArray)
    , GhostArray(ghostArray)
    , UsePointScalars(usePointScalars)
    , NumberOfCells(input->GetNumberOfCells())
    , KeptCellsList(keptCellsList)
  {
    this->InsidenessArray->SetNumberOfComponents(1);
    this->InsidenessArray->SetNumberOfTuples(this->NumberOfCells);
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

    auto scalars = vtk::DataArrayTupleRange(this->ScalarsArray);
    auto insideness = vtk::DataArrayValueRange<1>(this->InsidenessArray);

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

      if (this->GhostArray && this->GhostArray->GetValue(cellId) & vtkDataSetAttributes::HIDDENCELL)
      {
        insideness[cellId] = 0;
        continue;
      }
      int cellType = this->Input->GetCellType(cellId);
      if (cellType == VTK_EMPTY_CELL)
      {
        insideness[cellId] = 0;
        continue;
      }
      this->Input->GetCellPoints(cellId, numCellPts, cellPts, cellIds);

      int keepCell = 0;
      if (this->UsePointScalars)
      {
        if (this->Self->GetAllScalars())
        {
          keepCell = 1;
          for (int i = 0; keepCell && (i < numCellPts); i++)
          {
            keepCell = this->Self->EvaluateComponents(scalars, cellPts[i]);
          }
        }
        else
        {
          if (!this->Self->GetUseContinuousCellRange())
          {
            keepCell = 0;
            for (int i = 0; (!keepCell) && (i < numCellPts); i++)
            {
              keepCell = this->Self->EvaluateComponents(scalars, cellPts[i]);
            }
          }
          else
          {
            keepCell = this->Self->EvaluateCell(scalars, cellPts, numCellPts);
          }
        }
      }
      else // use cell scalars
      {
        keepCell = this->Self->EvaluateComponents(scalars, cellId);
      }
      // Invert the keep flag if the Invert option is enabled.
      keepCell = this->Self->GetInvert() ? (1 - keepCell) : keepCell;
      insideness[cellId] = numCellPts > 0 && keepCell ? 1 : 0;
    }
    if (isFirst)
    {
      this->Self->UpdateProgress(end * 1.0 / this->NumberOfCells);
    }
  }

  void Reduce()
  {
    this->KeptCellsList->Allocate(this->NumberOfCells);
    for (vtkIdType cellId = 0; cellId < this->NumberOfCells; ++cellId)
    {
      if (this->InsidenessArray->GetValue(cellId))
      {
        this->KeptCellsList->InsertNextId(cellId);
      }
    }
  }
};

//------------------------------------------------------------------------------
struct vtkThreshold::EvaluateCellsWorker
{
  template <typename TScalarArray>
  void operator()(TScalarArray* scalarsArray, vtkThreshold* self, vtkDataSet* input,
    vtkUnsignedCharArray* ghostArray, bool usePointScalars, vtkIdList* keptCellsList)
  {
    EvaluateCellsFunctor<TScalarArray> functor(
      self, input, scalarsArray, ghostArray, usePointScalars, keptCellsList);
    vtkSMPTools::For(0, input->GetNumberOfCells(), functor);
  }
};

//------------------------------------------------------------------------------
int vtkThreshold::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0]->GetInformationObject(0));

  vtkDebugMacro(<< "Executing threshold filter");

  vtkDataArray* inScalars = this->GetInputArrayToProcess(0, inputVector);

  if (!inScalars)
  {
    vtkDebugMacro(<< "No scalar data to threshold");
    return 1;
  }
  vtkUnsignedCharArray* ghostsArray = input->GetCellData()->GetGhostArray();

  // are we using pointScalars?
  int fieldAssociation = this->GetInputArrayAssociation(0, inputVector);
  this->NumberOfComponents = inScalars->GetNumberOfComponents();
  bool usePointScalars = fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS;

  auto keptCellsList = vtkSmartPointer<vtkIdList>::New(); // maps old point ids into new

  // The result is computed in two steps: EvaluateCellsWorker to select cells &
  // vtkExtractCells to extract them.  The fraction of the total time required
  // for these operations varies based on type of dataset (image vs
  // unstructured) and size of extracted region. To keep things simple we just
  // devote 50% to each step even if one of they two completes faster.
  this->SetProgressShiftScale(0, 0.5);
  EvaluateCellsWorker worker;
  if (!vtkArrayDispatch::Dispatch::Execute(
        inScalars, worker, this, input, ghostsArray, usePointScalars, keptCellsList))
  {
    worker(inScalars, this, input, ghostsArray, usePointScalars, keptCellsList);
  }
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
  extractCells->SetCellList(keptCellsList);
  extractCells->AssumeSortedAndUniqueIdsOn();
  extractCells->PassThroughCellIdsOff();
  extractCells->SetOutputPointsPrecision(this->OutputPointsPrecision);

  vtkNew<vtkEventForwarderCommand> progressForwarder;
  progressForwarder->SetTarget(this);
  extractCells->AddObserver(vtkCommand::ProgressEvent, progressForwarder);
  extractCells->SetProgressShiftScale(0.5, 0.5);
  this->SetProgressShiftScale(0.5, 0.5);

  return extractCells->ProcessRequest(request, inputVector, outputVector);
}

//------------------------------------------------------------------------------
template <typename TScalarsArray>
int vtkThreshold::EvaluateCell(
  TScalarsArray& scalars, const vtkIdType* cellPts, vtkIdType numCellPts)
{
  int c(0);
  int keepCell(0);
  switch (this->ComponentMode)
  {
    case VTK_COMPONENT_MODE_USE_SELECTED:
      c = this->SelectedComponent < this->NumberOfComponents ? this->SelectedComponent : 0;
      keepCell = EvaluateCell(scalars, c, cellPts, numCellPts);
      break;
    case VTK_COMPONENT_MODE_USE_ANY:
      keepCell = 0;
      for (c = 0; (!keepCell) && (c < this->NumberOfComponents); c++)
      {
        keepCell = EvaluateCell(scalars, c, cellPts, numCellPts);
      }
      break;
    case VTK_COMPONENT_MODE_USE_ALL:
      keepCell = 1;
      for (c = 0; keepCell && (c < this->NumberOfComponents); c++)
      {
        keepCell = EvaluateCell(scalars, c, cellPts, numCellPts);
      }
      break;
  }
  return keepCell;
}

//------------------------------------------------------------------------------
template <typename TScalarsArray>
int vtkThreshold::EvaluateCell(
  TScalarsArray& scalars, int c, const vtkIdType* cellPts, vtkIdType numCellPts)
{
  double minScalar = DBL_MAX, maxScalar = DBL_MIN;
  for (int i = 0; i < numCellPts; i++)
  {
    double s = scalars[cellPts[i]][c];
    minScalar = std::min(s, minScalar);
    maxScalar = std::max(s, maxScalar);
  }

  int keepCell = !(this->LowerThreshold > maxScalar || this->UpperThreshold < minScalar);
  return keepCell;
}

//------------------------------------------------------------------------------
template <typename TScalarsArray>
int vtkThreshold::EvaluateComponents(TScalarsArray& scalars, vtkIdType id)
{
  int keepCell = 0;
  int c;
  switch (this->ComponentMode)
  {
    case VTK_COMPONENT_MODE_USE_SELECTED:
    {
      double value = 0.0;
      if (!this->ComputeMagnitude(value, scalars, id))
      {
        c = this->SelectedComponent < this->NumberOfComponents ? this->SelectedComponent : 0;
        value = static_cast<double>(scalars[id][c]);
      }
      keepCell = (this->*(this->ThresholdFunction))(value);
    }
    break;
    case VTK_COMPONENT_MODE_USE_ANY:
      keepCell = 0;
      for (c = 0; (!keepCell) && (c < this->NumberOfComponents); c++)
      {
        keepCell = (this->*(this->ThresholdFunction))(static_cast<double>(scalars[id][c]));
      }
      break;
    case VTK_COMPONENT_MODE_USE_ALL:
      keepCell = 1;
      for (c = 0; keepCell && (c < this->NumberOfComponents); c++)
      {
        keepCell = (this->*(this->ThresholdFunction))(static_cast<double>(scalars[id][c]));
      }
      break;
  }
  return keepCell;
}

//------------------------------------------------------------------------------
void vtkThreshold::SetAttributeModeToDefault()
{
  this->SetAttributeMode(VTK_ATTRIBUTE_MODE_DEFAULT);
}

//------------------------------------------------------------------------------
void vtkThreshold::SetAttributeModeToUsePointData()
{
  this->SetAttributeMode(VTK_ATTRIBUTE_MODE_USE_POINT_DATA);
}

//------------------------------------------------------------------------------
void vtkThreshold::SetAttributeModeToUseCellData()
{
  this->SetAttributeMode(VTK_ATTRIBUTE_MODE_USE_CELL_DATA);
}

//------------------------------------------------------------------------------
// Return the method for manipulating scalar data as a string.
const char* vtkThreshold::GetAttributeModeAsString()
{
  if (this->AttributeMode == VTK_ATTRIBUTE_MODE_DEFAULT)
  {
    return "Default";
  }
  else if (this->AttributeMode == VTK_ATTRIBUTE_MODE_USE_POINT_DATA)
  {
    return "UsePointData";
  }
  else
  {
    return "UseCellData";
  }
}

//------------------------------------------------------------------------------
// Return a string representation of the component mode
const char* vtkThreshold::GetComponentModeAsString()
{
  if (this->ComponentMode == VTK_COMPONENT_MODE_USE_SELECTED)
  {
    return "UseSelected";
  }
  else if (this->ComponentMode == VTK_COMPONENT_MODE_USE_ANY)
  {
    return "UseAny";
  }
  else
  {
    return "UseAll";
  }
}

//------------------------------------------------------------------------------
void vtkThreshold::SetPointsDataTypeToDouble()
{
  this->SetPointsDataType(VTK_DOUBLE);
}

//------------------------------------------------------------------------------
void vtkThreshold::SetPointsDataTypeToFloat()
{
  this->SetPointsDataType(VTK_FLOAT);
}

//------------------------------------------------------------------------------
void vtkThreshold::SetPointsDataType(int type)
{
  if (type == VTK_FLOAT)
  {
    this->SetOutputPointsPrecision(SINGLE_PRECISION);
  }
  else if (type == VTK_DOUBLE)
  {
    this->SetOutputPointsPrecision(DOUBLE_PRECISION);
  }
}

//------------------------------------------------------------------------------
int vtkThreshold::GetPointsDataType()
{
  if (this->OutputPointsPrecision == SINGLE_PRECISION)
  {
    return VTK_FLOAT;
  }
  else if (this->OutputPointsPrecision == DOUBLE_PRECISION)
  {
    return VTK_DOUBLE;
  }

  return 0;
}

//------------------------------------------------------------------------------
int vtkThreshold::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
template <typename TScalarsArray>
bool vtkThreshold::ComputeMagnitude(double& magnitude, const TScalarsArray& scalars, vtkIdType id)
{
  if (this->SelectedComponent != this->NumberOfComponents || this->NumberOfComponents <= 1)
  {
    // If NumberOfComponents == 1, magnitude equals component value
    // so don't do extra computation
    return false;
  }

  double squaredNorm = 0.0;
  for (int i = 0; i < this->NumberOfComponents; ++i)
  {
    const double value = static_cast<double>(scalars[id][i]);
    squaredNorm += value * value;
  }

  magnitude = std::sqrt(squaredNorm);
  return true;
}

//------------------------------------------------------------------------------
void vtkThreshold::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Component Mode: " << this->GetComponentModeAsString() << endl;
  os << indent << "Selected Component: " << this->SelectedComponent << endl;

  os << indent << "All Scalars: " << this->AllScalars << "\n";
  if (this->ThresholdFunction == &vtkThreshold::Upper)
  {
    os << indent << "Threshold By Upper\n";
  }

  else if (this->ThresholdFunction == &vtkThreshold::Lower)
  {
    os << indent << "Threshold By Lower\n";
  }

  else if (this->ThresholdFunction == &vtkThreshold::Between)
  {
    os << indent << "Threshold Between\n";
  }

  os << indent << "Lower Threshold: " << this->LowerThreshold << "\n";
  os << indent << "Upper Threshold: " << this->UpperThreshold << "\n";
  os << indent << "Precision of the output points: " << this->OutputPointsPrecision << "\n";
  os << indent << "Use Continuous Cell Range: " << this->UseContinuousCellRange << endl;
}
VTK_ABI_NAMESPACE_END
