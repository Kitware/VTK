// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAnimateModes.h"

#include "vtkArrayDispatch.h"
#include "vtkAssume.h"
#include "vtkDataArrayAccessor.h"
#include "vtkDataObjectTree.h"
#include "vtkDataSetAttributes.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLogger.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"

VTK_ABI_NAMESPACE_BEGIN
struct vtkAnimateModesWorker
{
  template <typename PointsArray, typename DisplacementsArray>
  void operator()(PointsArray* inPoints, DisplacementsArray* inDisplacements,
    const double modeShapeTime, vtkDataArray* output, vtkAnimateModes* self)
  {
    auto outPoints = vtkArrayDownCast<PointsArray>(output);
    VTK_ASSUME(inPoints->GetNumberOfComponents() == outPoints->GetNumberOfComponents());
    VTK_ASSUME(inPoints->GetNumberOfComponents() == inDisplacements->GetNumberOfComponents());

    const auto numTuples = inPoints->GetNumberOfTuples();
    const int numComps = inPoints->GetNumberOfComponents();
    vtkDataArrayAccessor<PointsArray> ipts(inPoints);
    vtkDataArrayAccessor<PointsArray> opts(outPoints);
    vtkDataArrayAccessor<DisplacementsArray> disp(inDisplacements);

    auto scale = self->GetDisplacementMagnitude() * std::cos(2.0 * vtkMath::Pi() * modeShapeTime);
    if (self->GetDisplacementPreapplied())
    {
      scale = scale - 1.0;
    }

    vtkSMPTools::For(0, numTuples, [&](vtkIdType start, vtkIdType end) {
      bool isFirst = vtkSMPTools::GetSingleThread();
      for (vtkIdType cc = start; cc < end; ++cc)
      {
        if (isFirst)
        {
          self->CheckAbort();
        }
        if (self->GetAbortOutput())
        {
          break;
        }
        for (int comp = 0; comp < numComps; ++comp)
        {
          opts.Set(cc, comp, ipts.Get(cc, comp) + disp.Get(cc, comp) * scale);
        }
      }
    });
  }
};

vtkStandardNewMacro(vtkAnimateModes);
//----------------------------------------------------------------------------
vtkAnimateModes::vtkAnimateModes()
  : AnimateVibrations(true)
  , ModeShapesRange{ 1, 1 }
  , ModeShape{ 1 }
  , DisplacementMagnitude{ 1.0 }
  , DisplacementPreapplied(false)
  , TimeRange{ 0.0, 1.0 }
{
  // the displacement array
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::VECTORS);
}

//----------------------------------------------------------------------------
vtkAnimateModes::~vtkAnimateModes() = default;

//----------------------------------------------------------------------------
int vtkAnimateModes::RequestInformation(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto inInfo = inputVector[0]->GetInformationObject(0);
  auto outInfo = outputVector->GetInformationObject(0);

  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    const auto length = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    this->InputTimeSteps.resize(length);
    inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), this->InputTimeSteps.data());
    this->ModeShapesRange[0] = 1;
    this->ModeShapesRange[1] = length;
  }
  else
  {
    this->InputTimeSteps.clear();
    this->ModeShapesRange[0] = 1;
    this->ModeShapesRange[1] = 1;
  }

  outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  if (this->AnimateVibrations)
  {
    double trange[] = { 0.0, 1.0 };
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), trange, 2);
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkAnimateModes::RequestUpdateExtent(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*)
{
  auto inInfo = inputVector[0]->GetInformationObject(0);
  const int num_timesteps = static_cast<int>(this->InputTimeSteps.size());
  const int shape_index = this->ModeShape - 1;
  if (shape_index >= 0 && shape_index < num_timesteps)
  {
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), this->InputTimeSteps[shape_index]);
  }
  else
  {
    inInfo->Remove(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkAnimateModes::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObjectTree");
  return 1;
}

//----------------------------------------------------------------------------
int vtkAnimateModes::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto outInfo = outputVector->GetInformationObject(0);
  const auto modeShapeTime = outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP())
    ? outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP())
    : 0;

  // functor to animate block.
  auto executeBlock = [this, modeShapeTime](vtkPointSet* block) {
    auto displacement = this->GetInputArrayToProcess(0, block);
    if (!displacement)
    {
      // no input displacement array, nothing to do.
      return;
    }

    auto points = block->GetPoints();
    auto newPoints = vtkPoints::New(points->GetDataType());
    newPoints->SetNumberOfPoints(points->GetNumberOfPoints());

    using PointTypes = vtkArrayDispatch::Reals;
    using Dispatcher = vtkArrayDispatch::Dispatch2ByValueType<PointTypes, PointTypes>;

    vtkAnimateModesWorker worker;
    if (!Dispatcher::Execute(
          points->GetData(), displacement, worker, modeShapeTime, newPoints->GetData(), this))
    {
      worker(points->GetData(), displacement, modeShapeTime, newPoints->GetData(), this);
    }

    block->SetPoints(newPoints);
    newPoints->FastDelete();
  };

  auto outputDO = vtkDataObject::GetData(outputVector, 0);
  if (auto inputDT = vtkDataObjectTree::GetData(inputVector[0], 0))
  {
    auto outputDT = vtkDataObjectTree::SafeDownCast(outputDO);
    assert(outputDT);

    outputDT->ShallowCopy(inputDT);
    for (auto block : vtkCompositeDataSet::GetDataSets<vtkPointSet>(outputDT))
    {
      executeBlock(block);
    }
  }
  else if (auto inputPS = vtkPointSet::GetData(inputVector[0], 0))
  {
    auto outputPS = vtkPointSet::SafeDownCast(outputDO);
    assert(outputPS);

    outputPS->ShallowCopy(inputPS);
    executeBlock(outputPS);
  }

  // Add field data arrays to provide information about the mode shape
  // downstream.
  vtkNew<vtkIntArray> modeShape;
  modeShape->SetName("mode_shape");
  modeShape->SetNumberOfComponents(1);
  modeShape->SetNumberOfTuples(1);
  modeShape->SetTypedComponent(0, 0, this->ModeShape);

  vtkNew<vtkIntArray> modeShapeRange;
  modeShapeRange->SetName("mode_shape_range");
  modeShapeRange->SetNumberOfComponents(2);
  modeShapeRange->SetNumberOfTuples(1);
  modeShapeRange->SetTypedTuple(0, this->ModeShapesRange);

  outputDO->GetFieldData()->AddArray(modeShape);
  outputDO->GetFieldData()->AddArray(modeShapeRange);
  outputDO->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), modeShapeTime);
  return 1;
}

//----------------------------------------------------------------------------
void vtkAnimateModes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AnimateVibrations: " << this->AnimateVibrations << endl;
  os << indent << "ModeShapesRange: " << this->ModeShapesRange[0] << ", "
     << this->ModeShapesRange[1] << endl;
  os << indent << "ModeShape: " << this->ModeShape << endl;
  os << indent << "DisplacementMagnitude: " << this->DisplacementMagnitude << endl;
  os << indent << "DisplacementPreapplied: " << this->DisplacementPreapplied << endl;
  os << indent << "TimeRange: " << this->TimeRange[0] << ", " << this->TimeRange[1] << endl;
}
VTK_ABI_NAMESPACE_END
